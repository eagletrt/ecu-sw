#include "temperatures-api.h"
#include "eagletrt.h"
#include <string.h>
#include "can-primary-api.h"
#include "can-communication-api.h"
#include "inverters-api.h"

EAGLETRT_STATIC struct TemperaturesHandler temperatures_handler;

enum TemperaturesReturnCode temperatures_api_init(void) {
    memset(&temperatures_handler, 0, sizeof(temperatures_handler));
    return TEMPERATURES_RC_OK;
}

enum TemperaturesReturnCode temperatures_api_update(enum TemperaturesName temperature_name, float temperature_value) {
    if (temperature_name >= TEMPERATURES_NAME_COUNT) {
        return TEMPERATURES_RC_ERROR;
    }

    temperatures_handler.temperatures[temperature_name] = temperature_value;

    return TEMPERATURES_RC_OK;
}

EAGLETRT_STATIC enum TemperaturesReturnCode prv_temperatures_api_send_motor_temperatures(void) {
    union CanPrimaryMessages message;
    message.motortemperature = (struct CanPrimaryMotortemperature){
        .temperaturefl = temperatures_handler.temperatures[TEMPERATURES_NAME_MOTOR1],
        .temperaturefr = temperatures_handler.temperatures[TEMPERATURES_NAME_MOTOR2],
        .temperaturerl = temperatures_handler.temperatures[TEMPERATURES_NAME_MOTOR3],
        .temperaturerr = temperatures_handler.temperatures[TEMPERATURES_NAME_MOTOR4]
    };

    struct CanCommunicationFrame frame = {
        .id = CAN_PRIMARY_MESSAGE_FRAME_ID_MOTORTEMPERATURE,
        .length = can_primary_byte_size_motortemperature,
    };
    if (can_primary_api_serialize_from_id(frame.id, &message, frame.data) < 0) {
        return TEMPERATURES_RC_ERROR;
    }

    if (can_communication_api_add_to_tx(CAN_COMMUNICATION_NETWORK_PRIMARY, &frame) != CAN_COMMUNICATION_RC_OK) {
        return TEMPERATURES_RC_ERROR;
    }

    return TEMPERATURES_RC_OK;
}

EAGLETRT_STATIC enum TemperaturesReturnCode prv_temperatures_api_send_inverter_temperatures(void) {
    union CanPrimaryMessages message;
    message.invertertemperature = (struct CanPrimaryInvertertemperature){
        .temperaturefl = temperatures_handler.temperatures[TEMPERATURES_NAME_INVERTER1],
        .temperaturefr = temperatures_handler.temperatures[TEMPERATURES_NAME_INVERTER2],
        .temperaturerl = temperatures_handler.temperatures[TEMPERATURES_NAME_INVERTER3],
        .temperaturerr = temperatures_handler.temperatures[TEMPERATURES_NAME_INVERTER4]
    };

    struct CanCommunicationFrame frame = {
        .id = CAN_PRIMARY_MESSAGE_FRAME_ID_INVERTERTEMPERATURE,
        .length = can_primary_byte_size_invertertemperature,
    };
    if (can_primary_api_serialize_from_id(frame.id, &message, frame.data) < 0) {
        return TEMPERATURES_RC_ERROR;
    }

    if (can_communication_api_add_to_tx(CAN_COMMUNICATION_NETWORK_PRIMARY, &frame) != CAN_COMMUNICATION_RC_OK) {
        return TEMPERATURES_RC_ERROR;
    }

    return TEMPERATURES_RC_OK;
}

enum TemperaturesReturnCode temperatures_api_periodically_send_temperatures(uint32_t tick) {
    EAGLETRT_STATIC uint32_t last_sent_tick = 0;

    if (tick - last_sent_tick >= can_primary_cycle_time_motortemperature) {
        last_sent_tick = tick;

        for (enum EphorusWheel wheel = EPHORUS_WHEEL_FRONT_LEFT; wheel < EPHORUS_WHEEL_COUNT; wheel++) {
            const struct EphorusWheelTelemetry *tlm = inverters_api_get_wheel_telemetry(wheel);
            if (tlm == NULL) {
                return TEMPERATURES_RC_ERROR;
            }
            temperatures_handler.temperatures[wheel] = tlm->temp_switches_c;
            temperatures_handler.temperatures[wheel + 4] = tlm->temp_motor_c;
        }

        if (prv_temperatures_api_send_motor_temperatures() != TEMPERATURES_RC_OK) {
            return TEMPERATURES_RC_ERROR;
        }

        if (prv_temperatures_api_send_inverter_temperatures() != TEMPERATURES_RC_OK) {
            return TEMPERATURES_RC_ERROR;
        }
    }

    return TEMPERATURES_RC_OK;
}
