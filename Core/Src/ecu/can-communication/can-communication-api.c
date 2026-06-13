/*!
 * \file can-communication-api.c
 * \author Dorijan Di Zepp
 * \date 2026-06-13
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
 * \brief Helper function to resolve a specific CAN node index to its assigned PAL handle.
 */
EAGLETRT_STATIC struct PalHandler *prv_can_communication_get_pal_handler_by_node(enum CanCommunicationNode node) {
    switch (node) {
        case CAN_COMM_NODE_1:
            return can_comm_handler.pal_can1;
        case CAN_COMM_NODE_2:
            return can_comm_handler.pal_can2;
        case CAN_COMM_NODE_3:
            return can_comm_handler.pal_can3;
        default:
            return NULL;
    }
}

enum CanCommunicationReturnCode can_communication_api_init(struct PalHandler *can1,
                                                           struct PalHandler *can2,
                                                           struct PalHandler *can3) {
    if (can1 == NULL || can2 == NULL || can3 == NULL) {
        return CAN_COMM_RC_NULL_POINTER;
    }

    can_comm_handler.pal_can1 = can1;
    can_comm_handler.pal_can2 = can2;
    can_comm_handler.pal_can3 = can3;

    return CAN_COMM_RC_OK;
}

enum CanCommunicationReturnCode can_communication_api_add_to_rx(enum CanCommunicationNode node,
                                                                const uint8_t *buffer,
                                                                uint32_t length) {
    if (node >= CAN_COMM_NODE_COUNT) {
        return CAN_COMM_RC_INVALID_ARGUMENT;
    }

    if (buffer == NULL || length == 0U) {
        return CAN_COMM_RC_NULL_POINTER;
    }

    struct PalHandler *target_pal = prv_can_communication_get_pal_handler_by_node(node);
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
    if (can_comm_handler.pal_can1 == NULL ||
        can_comm_handler.pal_can2 == NULL ||
        can_comm_handler.pal_can3 == NULL) {
        return CAN_COMM_RC_NULL_POINTER;
    }

    for (int i = 0; i < (int)CAN_COMM_NODE_COUNT; ++i) {
        struct PalHandler *target_pal = prv_can_communication_get_pal_handler_by_node((enum CanCommunicationNode)i);

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

enum CanCommunicationReturnCode can_communication_api_add_to_tx(enum CanCommunicationNode node,
                                                                const uint8_t *buffer,
                                                                uint32_t length) {
    if (node >= CAN_COMM_NODE_COUNT) {
        return CAN_COMM_RC_INVALID_ARGUMENT;
    }

    if (buffer == NULL || length == 0U) {
        return CAN_COMM_RC_NULL_POINTER;
    }

    struct PalHandler *target_pal = prv_can_communication_get_pal_handler_by_node(node);
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
    if (can_comm_handler.pal_can1 == NULL ||
        can_comm_handler.pal_can2 == NULL ||
        can_comm_handler.pal_can3 == NULL) {
        return CAN_COMM_RC_NULL_POINTER;
    }

    enum PalReturnCode rc1 = pal_api_process_tx(can_comm_handler.pal_can1);
    enum PalReturnCode rc2 = pal_api_process_tx(can_comm_handler.pal_can2);
    enum PalReturnCode rc3 = pal_api_process_tx(can_comm_handler.pal_can3);

    // PAL returns PAL_RC_QUEUE_EMPTY if nothing was staged
    if ((rc1 != PAL_RC_OK && rc1 != PAL_RC_QUEUE_EMPTY) ||
        (rc2 != PAL_RC_OK && rc2 != PAL_RC_QUEUE_EMPTY) ||
        (rc3 != PAL_RC_OK && rc3 != PAL_RC_QUEUE_EMPTY)) {
        return CAN_COMM_RC_TRANSMISSION_ERROR;
    }

    return CAN_COMM_RC_OK;
}