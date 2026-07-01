/*!
 * \file can-communication-router-api.c
 * \author Dorijan Di Zepp
 * \date 2026-07-01
 * \brief Routing and deserialization stubs for incoming ECU network traffic.
 */

#include "can-communication-router-api.h"
#include "can-primary-api.h"
#include "can-bms-api.h"
#include "can-inverters-api.h"
#include "vehicle-api.h"
#include "pedals-api.h"

enum CanCommunicationReturnCode can_communication_router_api_receive_primary(struct CanCommunicationFrame *frame) {
    if (frame == nullptr) {
        return CAN_COMMUNICATION_RC_NULL_POINTER;
    }

    if (!can_primary_api_id_is_valid(frame->id)) {
        return CAN_COMMUNICATION_RC_INVALID_NETWORK;
    }

    // 1. Convert the runtime CAN ID to the corresponding enum message index
    int message_index = can_primary_api_index_from_id(frame->id);
    if (message_index < 0) {
        return CAN_COMMUNICATION_RC_ERROR;
    }

    // 2. Deserialize exactly ONCE here using the resolved index
    union CanPrimaryMessages message = { 0 };
    if (can_primary_api_deserialize_from_index(message_index, frame->data, &message) != 0) {
        return CAN_COMMUNICATION_RC_ERROR;
    }

    // 3. Jump straight into the business logic clean and fast
    switch (frame->id) {
        case CAN_PRIMARY_MESSAGE_FRAME_ID_STEERING_WHEEL_SET_ECU_STATUS: {
            if (message.steering_wheel_set_ecu_status.targetstatus ==
                CAN_PRIMARY_STEERING_WHEEL_SET_ECU_STATUS_TARGETSTATUS_READY) {
                vehicle_api_set_ts_on_requested(true);
            }
            break;
        }

        case CAN_PRIMARY_MESSAGE_FRAME_ID_HV_BMS_FEEDBACK_STATUS: {
            bool is_high = (message.hv_bms_feedback_status.tsover60v ==
                            CAN_PRIMARY_HV_BMS_FEEDBACK_STATUS_TSOVER60V_HIGH);
            vehicle_api_set_voltage_higher_than_60v(is_high);
            break;
        }

        case CAN_PRIMARY_MESSAGE_FRAME_ID_PEDALS_THROTTLE: {
            float travel_pct = 0.0F;
            if ((message.pedals_throttle.status == CAN_PRIMARY_PEDALS_THROTTLE_STATUS_OK) ||
                (message.pedals_throttle.status == CAN_PRIMARY_PEDALS_THROTTLE_STATUS_IMPLAUSIBILITY_RECOVERABLE)) {
                travel_pct = message.pedals_throttle.travel_pct;
            }
            pedals_api_set_throttle(travel_pct);
            break;
        }

        case CAN_PRIMARY_MESSAGE_FRAME_ID_PEDALS_BRAKE: {
            float travel_pct = message.pedals_brake.travel_pct;

            float brake_pressure = (message.pedals_brake.pressurefront_bar +
                                    message.pedals_brake.pressurerear_bar) /
                                   2.0F;

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

    if (frame == NULL) {
        return CAN_COMMUNICATION_RC_NULL_POINTER;
    }

    if (!can_inverters_api_id_is_valid(frame->id)) {
        return CAN_COMMUNICATION_RC_INVALID_NETWORK;
    }

    // TODO: add libcan deserialization based on received frame

    return CAN_COMMUNICATION_RC_OK;
}