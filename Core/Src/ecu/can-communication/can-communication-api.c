/*!
 * \file can-communication-api.c
 * \author Dorijan Di Zepp
 * \date 2026-06-14
 * \brief Core implementation for managing ECU CAN bus mailboxes via PAL.
 */

#include "can-communication-api.h"
#include "eagletrt-api.h"
#include <stddef.h>

/*!
 * \brief Internal module context handler instance.
 */
EAGLETRT_STATIC struct CanCommunicationHandler can_comm_handler;

/*!
 * \brief Helper function to resolve a specific CAN network index to its assigned PAL handle.
 */
EAGLETRT_STATIC struct PalHandler *prv_can_communication_get_pal_handler_by_network_id(enum CanCommunicationNetwork network_id) {
    switch (network_id) {
        case CAN_COMM_NET_PRIMARY:
            return can_comm_handler.primary_pal;
        case CAN_COMM_NET_SECONDARY:
            return can_comm_handler.secondary_pal;
        case CAN_COMM_NET_INVERTER:
            return can_comm_handler.inverter_pal;
        default:
            return NULL;
    }
}

enum CanCommunicationReturnCode can_communication_api_init(struct PalHandler *primary_pal,
                                                           struct PalHandler *secondary_pal,
                                                           struct PalHandler *inverter_pal) {
    if (primary_pal == NULL || secondary_pal == NULL || inverter_pal == NULL) {
        return CAN_COMM_RC_NULL_POINTER;
    }

    can_comm_handler.primary_pal = primary_pal;
    can_comm_handler.secondary_pal = secondary_pal;
    can_comm_handler.inverter_pal = inverter_pal;

    return CAN_COMM_RC_OK;
}

enum CanCommunicationReturnCode can_communication_api_add_to_rx(enum CanCommunicationNetwork network_id,
                                                                const uint8_t *buffer,
                                                                uint32_t length) {
    if (network_id >= CAN_COMM_NET_COUNT) {
        return CAN_COMM_RC_INVALID_ARGUMENT;
    }

    if (buffer == NULL || length == 0U) {
        return CAN_COMM_RC_NULL_POINTER;
    }

    struct PalHandler *target_pal = prv_can_communication_get_pal_handler_by_network_id(network_id);
    if (target_pal == NULL) {
        return CAN_COMM_RC_NULL_POINTER;
    }

    // Cast explicitly to (uint8_t *) to safely handle the const pointer
    enum PalReturnCode pal_rc = pal_api_add_to_rx_queue(target_pal, (uint8_t *)buffer, length);
    if (pal_rc != PAL_RC_OK) {
        if (pal_rc == PAL_RC_QUEUE_FULL) {
            return CAN_COMM_RC_BUFFER_FULL;
        }
        return CAN_COMM_RC_ERROR;
    }

    return CAN_COMM_RC_OK;
}

enum CanCommunicationReturnCode can_communication_api_process_rx(void *application_state) {
    if (can_comm_handler.primary_pal == NULL ||
        can_comm_handler.secondary_pal == NULL ||
        can_comm_handler.inverter_pal == NULL) {
        return CAN_COMM_RC_NULL_POINTER;
    }

    for (int i = 0; i < (int)CAN_COMM_NET_COUNT; ++i) {
        struct PalHandler *target_pal = prv_can_communication_get_pal_handler_by_network_id((enum CanCommunicationNetwork)i);

        if (target_pal == NULL) {
            continue;
        }

        enum PalReturnCode pal_rc = PAL_RC_OK;

        while (pal_rc != PAL_RC_QUEUE_EMPTY) {
            /* 
             * Forward the explicit application state target right into PAL. 
             * PAL will automatically deliver it to the registered callback.
             */
            pal_rc = pal_api_process_rx(target_pal, application_state);

            if (pal_rc != PAL_RC_OK && pal_rc != PAL_RC_QUEUE_EMPTY) {
                return CAN_COMM_RC_ERROR;
            }
        }
    }

    return CAN_COMM_RC_OK;
}

enum CanCommunicationReturnCode can_communication_api_add_to_tx(enum CanCommunicationNetwork network_id,
                                                                const uint8_t *buffer,
                                                                uint32_t length) {
    if (network_id >= CAN_COMM_NET_COUNT) {
        return CAN_COMM_RC_INVALID_ARGUMENT;
    }

    if (buffer == NULL || length == 0U) {
        return CAN_COMM_RC_NULL_POINTER;
    }

    struct PalHandler *target_pal = prv_can_communication_get_pal_handler_by_network_id(network_id);
    if (target_pal == NULL) {
        return CAN_COMM_RC_NULL_POINTER;
    }

    // Cast explicitly to (void *) to pass a const pointer to a generic void pointer parameter
    enum PalReturnCode pal_rc = pal_api_add_to_tx_queue(target_pal, (void *)buffer, length);
    if (pal_rc != PAL_RC_OK) {
        if (pal_rc == PAL_RC_QUEUE_FULL) {
            return CAN_COMM_RC_BUFFER_FULL;
        }
        return CAN_COMM_RC_ERROR;
    }

    return CAN_COMM_RC_OK;
}

enum CanCommunicationReturnCode can_communication_api_process_tx(void) {
    if (can_comm_handler.primary_pal == NULL ||
        can_comm_handler.secondary_pal == NULL ||
        can_comm_handler.inverter_pal == NULL) {
        return CAN_COMM_RC_NULL_POINTER;
    }

    for (int i = 0; i < (int)CAN_COMM_NET_COUNT; ++i) {
        struct PalHandler *target_pal = prv_can_communication_get_pal_handler_by_network_id((enum CanCommunicationNetwork)i);

        if (target_pal == NULL) {
            continue;
        }

        enum PalReturnCode pal_rc = PAL_RC_OK;

        // Keep popping from the PAL queue until it is completely empty
        while (pal_rc != PAL_RC_QUEUE_EMPTY) {

            pal_rc = pal_api_process_tx(target_pal);

            // If it's not OK and not EMPTY, something went fundamentally wrong in the hardware
            if (pal_rc != PAL_RC_OK && pal_rc != PAL_RC_QUEUE_EMPTY) {
                return CAN_COMM_RC_ERROR;
            }
        }
    }

    return CAN_COMM_RC_OK;
}