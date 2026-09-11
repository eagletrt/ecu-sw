/*!
 * \file can-communication-router-api.c
 * \author Dorijan Di Zepp
 * \date 2026-07-01
 * \brief Routing and deserialization stubs for incoming ECU network traffic.
 */

#include "can-communication-router-api.h"
#include "can-primary-api.h"
#include "inverters-api.h"
#include "lights-api.h"
#include "tsac-api.h"
#include "pedals-api.h"
#include "buzzer-api.h"
#include "vehicle-api.h"
#include "eagletrt.h"

enum CanCommunicationReturnCode can_communication_router_api_receive_primary(struct CanCommunicationFrame *frame) {
    if (frame == NULL) {
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
            vehicle_api_set_ts_on_button_pressed(message.steeringwheelbuttonstatus.tson != 0);
            vehicle_api_set_ptt(message.steeringwheelbuttonstatus.ptt != 0);
            break;
        }

        case CAN_PRIMARY_MESSAGE_FRAME_ID_TSACSTATUS: {
            tsac_api_set_tsac_status(message.tsacstatus.mainboardstatus);
            break;
        }

        case CAN_PRIMARY_MESSAGE_FRAME_ID_TSACMAINBOARDFEEDBACK: {
            bool is_high = (message.tsacmainboardfeedback.tslessthan60v == 0);
            tsac_api_set_voltage_higher_than_60v(is_high);
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
            /*
            constexpr float pressure_count = 2.0F;

            float brake_pressure = (message.pedalsbrake.pressurefl +
                                    message.pedalsbrake.pressurerr) /
                                   pressure_count;
            */

            pedals_api_set_brake(travel_pct);
            pedals_api_set_brake_pressure(message.pedalsbrake.pressurefl);
            lights_api_set_light_state(LIGHTS_NAME_BRAKE, pedals_api_is_brake_pressed());
            break;
        }

        case CAN_PRIMARY_MESSAGE_FRAME_ID_TSACMAINBOARDESTIMATEDSOC: {
            inverters_api_set_soc(message.tsacmainboardestimatedsoc.soc);
            break;
        }

        case CAN_PRIMARY_MESSAGE_FRAME_ID_TELEMETRYFSM: {
            // Beep once whenever telemetry logging (the RUN state) starts or stops, so
            // the driver gets an audible cue independently of the ECU's own FSM state.
            // The edge is detected against the previous status; the 1 s tone plays on
            // the ASSI buzzer and is serviced by the main-loop buzzer poll, which is
            // why a single request() here is enough.
            EAGLETRT_STATIC uint8_t previous_status = CAN_PRIMARY_TELEMETRYFSM_STATUS_INIT;
            uint8_t status = message.telemetryfsm.status;

            bool was_logging = (previous_status == CAN_PRIMARY_TELEMETRYFSM_STATUS_RUN);
            bool is_logging = (status == CAN_PRIMARY_TELEMETRYFSM_STATUS_RUN);
            if (was_logging != is_logging) {
                constexpr uint32_t telemetry_beep_ms = 1000;
                constexpr uint32_t telemetry_beep_frequency_hz = 1000;
                constexpr float telemetry_beep_amplitude = 0.33F;
                // ASSI is a PWM buzzer, so set frequency/amplitude here to keep the
                // tone self-contained rather than relying on values set elsewhere.
                buzzer_api_set_frequency(BUZZER_TYPE_ASSI, telemetry_beep_frequency_hz);
                buzzer_api_set_amplitude(BUZZER_TYPE_ASSI, telemetry_beep_amplitude);
                buzzer_api_set_duration(BUZZER_TYPE_ASSI, telemetry_beep_ms);
                buzzer_api_request(BUZZER_TYPE_ASSI);
            }
            previous_status = status;
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
