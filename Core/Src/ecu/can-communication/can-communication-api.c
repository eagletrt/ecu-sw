/*!
 * \file can-communication.c
 * \author Dorijan Di Zepp
 * \date 2026-06-20
 * \brief Core implementation for managing ECU CAN bus mailboxes via PAL.
 */

#include "can-communication-api.h"
#include "arena-allocator-api.h"
#include "pal-api.h"
#include "eagletrt-api.h"

#include <string.h>

/*!
 * \brief Total size of the encoded frame buffer passed downstream through PAL.
 *
 * \details Sized exactly as: sizeof(uint32_t) + sizeof(uint8_t) + 8 = 13 bytes.
 */
#define CAN_COMM_RAW_FRAME_SIZE (sizeof(uint32_t) + sizeof(uint8_t) + CAN_COMMUNICATION_FRAME_DATA_SIZE)

/*!
 * \brief Global module instance tracking active networks and the memory arena.
 */
EAGLETRT_STATIC struct CanCommunicationHandler can_comm_handler;

/*!
 * \brief Serializes a high-level \ref CanCommunicationFrame into a flat, endian-safe byte stream.
 */
EAGLETRT_STATIC void prv_can_communication_encode_frame(const struct CanCommunicationFrame *frame, uint8_t *raw_frame) {
    uint16_t offset = 0U;

    // Copy ID
    (void)memcpy(raw_frame + offset, &frame->id, sizeof(frame->id));
    offset += (uint16_t)sizeof(frame->id);

    // Copy length
    (void)memcpy(raw_frame + offset, &frame->length, sizeof(frame->length));
    offset += (uint16_t)sizeof(frame->length);

    // Copy payload
    (void)memcpy(raw_frame + offset, frame->data, CAN_COMMUNICATION_FRAME_DATA_SIZE);
}

/*!
 * \brief Deserializes a flat byte stream back into a high-level \ref CanCommunicationFrame structure.
 */
EAGLETRT_STATIC void prv_can_communication_decode_frame(const uint8_t *raw_frame, struct CanCommunicationFrame *frame) {
    uint16_t offset = 0U;

    // Extract ID
    (void)memcpy(&frame->id, raw_frame + offset, sizeof(frame->id));
    offset += (uint16_t)sizeof(frame->id);

    // Extract length
    (void)memcpy(&frame->length, raw_frame + offset, sizeof(frame->length));
    offset += (uint16_t)sizeof(frame->length);

    // Clip frame length securely to the maximum allowable payload size boundary
    frame->length = EAGLETRT_API_MIN(frame->length, CAN_COMMUNICATION_FRAME_DATA_SIZE);

    // Extract payload
    (void)memcpy(frame->data, raw_frame + offset, CAN_COMMUNICATION_FRAME_DATA_SIZE);
}

/*!
 * \brief Unpacks raw PAL queue byte buffers and forwards them directly to the user hardware send routines.
 */
EAGLETRT_STATIC enum PalReturnCode prv_can_communication_hardware_tx_bridge(enum CanCommunicationNetwork network, const struct PalMessage *msg) {
    if (msg == NULL) {
        return PAL_RC_NULL_POINTER;
    }
    if (msg->size != CAN_COMM_RAW_FRAME_SIZE) {
        return PAL_RC_INVALID_ARGUMENT;
    }

    struct CanCommunicationFrame frame;
    prv_can_communication_decode_frame(msg->payload, &frame);

    const CanCommunicationSendCallback user_send = can_comm_handler.networks[network].send;
    if (user_send == NULL) {
        return PAL_RC_NULL_POINTER;
    }
    if (user_send(&frame) != CAN_COMM_RC_OK) {
        return PAL_RC_IO_ERROR;
    }
    return PAL_RC_OK;
}

/* Localized, descriptive bridge adapters that route downstream PAL calls to the shared transmitter logic */
EAGLETRT_STATIC enum PalReturnCode prv_can_communication_bridge_tx_primary(const struct PalMessage *msg) {
    return prv_can_communication_hardware_tx_bridge(CAN_COMM_NET_PRIMARY, msg);
}

EAGLETRT_STATIC enum PalReturnCode prv_can_communication_bridge_tx_secondary(const struct PalMessage *msg) {
    return prv_can_communication_hardware_tx_bridge(CAN_COMM_NET_SECONDARY, msg);
}

EAGLETRT_STATIC enum PalReturnCode prv_can_communication_bridge_tx_inverter(const struct PalMessage *msg) {
    return prv_can_communication_hardware_tx_bridge(CAN_COMM_NET_INVERTER, msg);
}

/*!
 * \brief PAL processing deserializer hook used to extract raw items out of RX byte buffers.
 */
EAGLETRT_STATIC enum PalReturnCode prv_can_communication_pal_deserialize(const struct PalMessage *message, void *frame_out) {
    if (message == NULL || frame_out == NULL) {
        return PAL_RC_NULL_POINTER;
    }
    if (message->size != CAN_COMM_RAW_FRAME_SIZE) {
        return PAL_RC_DESERIALIZATION_ERROR;
    }
    prv_can_communication_decode_frame(message->payload, (struct CanCommunicationFrame *)frame_out);
    return PAL_RC_OK;
}

/*!
 * \brief Central pipeline processing helper to execute enqueuing transactions into target PAL tracks.
 */
EAGLETRT_STATIC enum CanCommunicationReturnCode prv_can_communication_enqueue(enum CanCommunicationNetwork network, const struct CanCommunicationFrame *frame, bool to_tx) {
    if (frame == NULL) {
        return CAN_COMM_RC_NULL_POINTER;
    }
    if (network >= CAN_COMM_NET_COUNT) {
        return CAN_COMM_RC_INVALID_ARGUMENT;
    }
    if (frame->length > CAN_COMMUNICATION_FRAME_DATA_SIZE) {
        return CAN_COMM_RC_INVALID_LENGTH;
    }

    if (can_comm_handler.networks[network].send == NULL ||
        can_comm_handler.networks[network].on_receive == NULL) {
        return CAN_COMM_RC_NOT_INITIALIZED;
    }

    uint8_t raw_frame[CAN_COMM_RAW_FRAME_SIZE];
    prv_can_communication_encode_frame(frame, raw_frame);

    const enum PalReturnCode return_code = to_tx
                                               ? pal_api_add_to_tx_queue(&can_comm_handler.networks[network].pal, raw_frame, CAN_COMM_RAW_FRAME_SIZE)
                                               : pal_api_add_to_rx_queue(&can_comm_handler.networks[network].pal, raw_frame, CAN_COMM_RAW_FRAME_SIZE);

    switch (return_code) {
        case PAL_RC_OK:
            return CAN_COMM_RC_OK;
        case PAL_RC_QUEUE_FULL:
            return CAN_COMM_RC_BUFFER_FULL;
        case PAL_RC_NULL_POINTER:
            return CAN_COMM_RC_NULL_POINTER;
        default:
            return CAN_COMM_RC_ERROR;
    }
}

/* =========================================================================
 * PUBLIC API FUNCTIONS
 * ========================================================================= */

enum CanCommunicationReturnCode can_communication_api_init(enum CanCommunicationNetwork network, const struct CanCommunicationNetworkConfig *config) {
    if (config == NULL || config->send == NULL || config->on_receive == NULL) {
        return CAN_COMM_RC_NULL_POINTER;
    }
    if (network >= CAN_COMM_NET_COUNT) {
        return CAN_COMM_RC_INVALID_ARGUMENT;
    }

    memset(&can_comm_handler.networks[network], 0, sizeof(can_comm_handler.networks[network]));

    can_comm_handler.networks[network].send = config->send;
    can_comm_handler.networks[network].on_receive = config->on_receive;

    /* Explicit inline resolution to map the appropriate transmission bridge callback */
    pal_send_callback active_tx_bridge = NULL;
    switch (network) {
        case CAN_COMM_NET_PRIMARY:
            active_tx_bridge = prv_can_communication_bridge_tx_primary;
            break;
        case CAN_COMM_NET_SECONDARY:
            active_tx_bridge = prv_can_communication_bridge_tx_secondary;
            break;
        case CAN_COMM_NET_INVERTER:
            active_tx_bridge = prv_can_communication_bridge_tx_inverter;
            break;
        default:
            return CAN_COMM_RC_INVALID_ARGUMENT;
    }

    if (pal_api_init(
            &can_comm_handler.networks[network].pal,
            CAN_COMMUNICATION_RX_QUEUE_CAPACITY,
            CAN_COMMUNICATION_TX_QUEUE_CAPACITY,
            CAN_COMM_RAW_FRAME_SIZE,
            prv_can_communication_pal_deserialize,
            active_tx_bridge,
            config->cs_enter,
            config->cs_exit,
            &can_comm_handler.arena) != PAL_RC_OK) {

        memset(&can_comm_handler.networks[network], 0, sizeof(can_comm_handler.networks[network]));
        return CAN_COMM_RC_ERROR;
    }

    return CAN_COMM_RC_OK;
}

enum CanCommunicationReturnCode can_communication_api_add_to_tx(enum CanCommunicationNetwork network, const struct CanCommunicationFrame *frame) {
    return prv_can_communication_enqueue(network, frame, true);
}

enum CanCommunicationReturnCode can_communication_api_add_to_rx(enum CanCommunicationNetwork network, const struct CanCommunicationFrame *frame) {
    return prv_can_communication_enqueue(network, frame, false);
}

enum CanCommunicationReturnCode can_communication_api_process_tx(enum CanCommunicationNetwork network) {
    if (network >= CAN_COMM_NET_COUNT) {
        return CAN_COMM_RC_INVALID_ARGUMENT;
    }
    if (can_comm_handler.networks[network].send == NULL ||
        can_comm_handler.networks[network].on_receive == NULL) {
        return CAN_COMM_RC_NOT_INITIALIZED;
    }

    enum CanCommunicationReturnCode result = CAN_COMM_RC_OK;
    enum PalReturnCode return_code;

    // Loop until queue empty or other
    do {
        return_code = pal_api_process_tx(&can_comm_handler.networks[network].pal);

        if (return_code == PAL_RC_IO_ERROR) {
            result = CAN_COMM_RC_TRANSMISSION_ERROR;
        } else if (return_code != PAL_RC_OK && return_code != PAL_RC_QUEUE_EMPTY) {
            return CAN_COMM_RC_ERROR;
        }
    } while (return_code != PAL_RC_QUEUE_EMPTY);

    return result;
}

enum CanCommunicationReturnCode can_communication_api_process_rx(enum CanCommunicationNetwork network) {
    if (network >= CAN_COMM_NET_COUNT) {
        return CAN_COMM_RC_INVALID_ARGUMENT;
    }
    if (can_comm_handler.networks[network].send == NULL ||
        can_comm_handler.networks[network].on_receive == NULL) {
        return CAN_COMM_RC_NOT_INITIALIZED;
    }

    const CanCommunicationReceiveCallback dispatcher = can_comm_handler.networks[network].on_receive;
    enum CanCommunicationReturnCode result = CAN_COMM_RC_OK;
    enum PalReturnCode return_code;

    // Loop until queue empty or other
    do {
        struct CanCommunicationFrame frame;
        return_code = pal_api_process_rx(&can_comm_handler.networks[network].pal, &frame);

        if (return_code == PAL_RC_OK) {
            if (dispatcher(&frame) != CAN_COMM_RC_OK) {
                result = CAN_COMM_RC_RECEIVE_HANDLER_ERROR;
            }
        } else if (return_code != PAL_RC_QUEUE_EMPTY) {
            return CAN_COMM_RC_ERROR;
        }
    } while (return_code != PAL_RC_QUEUE_EMPTY);

    return result;
}