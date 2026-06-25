/*!
 * \file can-communication-router-api.c
 * \author Dorijan Di Zepp
 * \date 2026-06-24
 * \brief Routing and deserialization stubs for incoming ECU network traffic.
 */

#include "can-communication-router-api.h"

enum CanCommunicationReturnCode can_communication_router_api_receive_primary(const struct CanCommunicationFrame *frame) {
    if (frame == NULL) {
        return CAN_COMMUNICATION_RC_NULL_POINTER;
    }

    // TODO: add libcan deserialization and vehicle state dispatch logic here
    // e.g., switch (frame->id) { ... }

    return CAN_COMMUNICATION_RC_OK;
}

enum CanCommunicationReturnCode can_communication_router_api_receive_secondary(const struct CanCommunicationFrame *frame) {
    if (frame == NULL) {
        return CAN_COMMUNICATION_RC_NULL_POINTER;
    }

    // TODO: add libcan deserialization and vehicle state dispatch logic here
    // e.g., switch (frame->id) { ... }

    return CAN_COMMUNICATION_RC_OK;
}

enum CanCommunicationReturnCode can_communication_router_api_receive_inverter(const struct CanCommunicationFrame *frame) {
    if (frame == NULL) {
        return CAN_COMMUNICATION_RC_NULL_POINTER;
    }

    // TODO: add libcan deserialization and powertrain state dispatch logic here
    // e.g., switch (frame->id) { ... }

    return CAN_COMMUNICATION_RC_OK;
}