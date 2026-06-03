/*!
 * \file as-driver-api.c
 * \author Dorijan Di Zepp
 * \date 2026-06-04
 * \brief Implementation of the global AS Driver module.
 * \details Provides an interface to store all required flags and values
 * needed to verify the correct behavior and safety of the AS driver.
 */

#include "as-driver-api.h"
#include "eagletrt-api.h"
#include <string.h>

/*!
 * \brief Internal module handler.
 * \details Hidden from external linkage to enforce API-only access.
 */
EAGLETRT_STATIC struct ASDriverHandler as_driver_handler;

enum ASDriverReturnCode as_driver_api_init(air_release_from_line_callback air_callback) {
    if (air_callback == NULL) {
        return AS_DRIVER_RC_ERROR;
    }

    // Clear all fields to 0, false and default struct parameters safely
    (void)memset(&as_driver_handler, 0, sizeof(struct ASDriverHandler));

    // Populate safe startup parameters and bindings
    as_driver_handler.as_mission = AS_DRIVER_MISSION_NOT_SELECTED;
    as_driver_handler.res_signal = AS_DRIVER_RES_SIGNAL_NONE;
    as_driver_handler.watchdog_state = AS_DRIVER_WATCHDOG_STATE_UNTESTED;
    as_driver_handler.release_air = air_callback;

    return AS_DRIVER_RC_OK;
}

enum ASDriverReturnCode as_driver_api_release_air(enum ASDriverAirLine line) {
    if (as_driver_handler.release_air == NULL) {
        return AS_DRIVER_RC_ERROR;
    }

    return as_driver_handler.release_air(line);
}

void as_driver_api_set_mission(enum ASDriverMission mission) {
    as_driver_handler.as_mission = mission;
}

enum ASDriverMission as_driver_api_get_mission(void) {
    return as_driver_handler.as_mission;
}

void as_driver_api_set_brake_pressure(enum ASDriverBrakePressure index, float pressure) {
    if (index < AS_DRIVER_BRAKE_PRESSURE_COUNT) {
        as_driver_handler.brake_pressures[index] = pressure;
    }
}

enum ASDriverReturnCode as_driver_api_set_all_brake_pressures(const float pressures[AS_DRIVER_BRAKE_PRESSURE_COUNT]) {
    if (pressures == NULL) {
        return AS_DRIVER_RC_ERROR;
    }

    (void)memcpy(as_driver_handler.brake_pressures, pressures, sizeof(float) * AS_DRIVER_BRAKE_PRESSURE_COUNT);
    return AS_DRIVER_RC_OK;
}

float as_driver_api_get_brake_pressure(enum ASDriverBrakePressure index) {
    if (index >= AS_DRIVER_BRAKE_PRESSURE_COUNT) {
        return 0.0F; // Safety fallback
    }
    return as_driver_handler.brake_pressures[index];
}

const float *as_driver_api_get_all_brake_pressures(void) {
    return as_driver_handler.brake_pressures;
}

void as_driver_api_set_mechanical_sensor(enum ASDriverMechanicalSensor index, float value) {
    if (index < AS_DRIVER_MECHANICAL_SENSOR_COUNT) {
        as_driver_handler.mechanical_sensors[index] = value;
    }
}

enum ASDriverReturnCode as_driver_api_set_all_mechanical_sensors(const float sensors[AS_DRIVER_MECHANICAL_SENSOR_COUNT]) {
    if (sensors == NULL) {
        return AS_DRIVER_RC_ERROR;
    }

    (void)memcpy(as_driver_handler.mechanical_sensors, sensors, sizeof(float) * AS_DRIVER_MECHANICAL_SENSOR_COUNT);
    return AS_DRIVER_RC_OK;
}

float as_driver_api_get_mechanical_sensor(enum ASDriverMechanicalSensor index) {
    if (index >= AS_DRIVER_MECHANICAL_SENSOR_COUNT) {
        return 0.0F; // Safety fallback
    }
    return as_driver_handler.mechanical_sensors[index];
}

const float *as_driver_api_get_all_mechanical_sensors(void) {
    return as_driver_handler.mechanical_sensors;
}

void as_driver_api_set_res_signal(enum ASDriverRESSignal signal) {
    as_driver_handler.res_signal = signal;
}

enum ASDriverRESSignal as_driver_api_get_res_signal(void) {
    return as_driver_handler.res_signal;
}

void as_driver_api_set_watchdog_state(enum ASDriverWatchdogState state) {
    as_driver_handler.watchdog_state = state;
}

enum ASDriverWatchdogState as_driver_api_get_watchdog_state(void) {
    return as_driver_handler.watchdog_state;
}

void as_driver_api_set_asms_on(bool status) {
    as_driver_handler.is_asms_on = status;
}

bool as_driver_api_get_asms_on(void) {
    return as_driver_handler.is_asms_on;
}

void as_driver_api_set_mission_started(bool status) {
    as_driver_handler.is_mission_started = status;
}

bool as_driver_api_get_mission_started(void) {
    return as_driver_handler.is_mission_started;
}

void as_driver_api_set_tsms_on(bool status) {
    as_driver_handler.is_tsms_on = status;
}

bool as_driver_api_get_tsms_on(void) {
    return as_driver_handler.is_tsms_on;
}

void as_driver_api_set_sdc_closed(bool status) {
    as_driver_handler.is_sdc_closed = status;
}

bool as_driver_api_get_sdc_closed(void) {
    return as_driver_handler.is_sdc_closed;
}

void as_driver_api_set_ebs_active(bool status) {
    as_driver_handler.is_ebs_active = status;
}

bool as_driver_api_get_ebs_active(void) {
    return as_driver_handler.is_ebs_active;
}

void as_driver_api_set_standstill(bool is_standstill) {
    as_driver_handler.is_vehicle_standstill = is_standstill;
}

bool as_driver_api_get_standstill(void) {
    return as_driver_handler.is_vehicle_standstill;
}