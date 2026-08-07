/*!
 * \file can-communication-router-api.c
 * \author Dorijan Di Zepp
 * \date 2026-07-01
 * \brief Routing and deserialization stubs for incoming ECU network traffic.
 */

#include "can-communication-router-api.h"
#include "can-primary-api.h"
#include "inverters-api.h"
#include "vehicle-api.h"
#include "pedals-api.h"

enum CanCommunicationReturnCode can_communication_router_api_receive_primary(struct CanCommunicationFrame *frame) {
    if (frame == nullptr) {
        return CAN_COMMUNICATION_RC_NULL_POINTER;
    }

    if (!can_primary_api_id_is_valid(frame->id)) {
        return CAN_COMMUNICATION_RC_INVALID_NETWORK;
    }

    union CanPrimaryMessages message = { 0 };
    if (can_primary_api_deserialize_from_id(frame->id, frame->data, &message) != 0) {
        return CAN_COMMUNICATION_RC_ERROR;
    }

    switch (frame->id) {
        case CAN_PRIMARY_MESSAGE_FRAME_ID_STEERINGWHEELBUTTONSTATUS: {
            vehicle_api_set_ts_on_button_pressed(message.steeringwheelbuttonstatus.tson);
            break;
        }

        case CAN_PRIMARY_MESSAGE_FRAME_ID_TSACMAINBOARDFEEDBACK: {
            bool is_high = (message.tsacmainboardfeedback.tslessthan60v == 0);
            vehicle_api_set_voltage_higher_than_60v(is_high);
            break;
        }

        case CAN_PRIMARY_MESSAGE_FRAME_ID_PEDALSTHROTTLE: {
            float travel_pct = 0.0F;
            if ((message.pedalsthrottle.plausibility == CAN_PRIMARY_PEDALSTHROTTLE_PLAUSIBILITY_OK) ||
                (message.pedalsthrottle.plausibility == CAN_PRIMARY_PEDALSTHROTTLE_PLAUSIBILITY_IMPLAUSIBILITY_RECOVERABLE)) {
                travel_pct = message.pedalsthrottle.travel;
            }
            pedals_api_set_throttle(travel_pct);
            break;
        }

        case CAN_PRIMARY_MESSAGE_FRAME_ID_PEDALSBRAKE: {
            float travel_pct = message.pedalsbrake.travel;
            constexpr float pressure_count = 2.0F;

            float brake_pressure = (message.pedalsbrake.pressurefl +
                                    message.pedalsbrake.pressurerr) /
                                   pressure_count;

            pedals_api_set_brake(travel_pct);
            pedals_api_set_brake_pressure(brake_pressure);
            break;
        }

        default:
            break;
    }

    return CAN_COMMUNICATION_RC_OK;
}

enum CanCommunicationReturnCode can_communication_router_api_receive_secondary(struct CanCommunicationFrame *frame) {
    if (frame == NULL) {
        return CAN_COMMUNICATION_RC_NULL_POINTER;
    }

    // TODO: add libcan deserialization and vehicle state dispatch logic here
    // e.g., switch (frame->id) { ... }

    return CAN_COMMUNICATION_RC_OK;
}

enum CanCommunicationReturnCode can_communication_router_api_receive_inverter(struct CanCommunicationFrame *frame) {
    if (frame == NULL) {
        return CAN_COMMUNICATION_RC_NULL_POINTER;
    }

    // The inverters module decodes the frame and updates the driver telemetry.
    // Frames that are not part of the inverters network are ignored internally.
    return inverters_api_on_receive(frame);
}
