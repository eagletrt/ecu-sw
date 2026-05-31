/*!
 * \file as-driver-api.c
 * \author Dorijan Di Zepp
 * \date 2026-05-31
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

    // Clear all fields to 0, false and default struct parameters
    memset(&as_driver_handler, 0, sizeof(struct ASDriverHandler));

    // Populate safe startup parameters and bindings
    as_driver_handler.as_mission = AS_DRIVER_MISSION_NOT_SELECTED;
    as_driver_handler.release_air = air_callback;

    return AS_DRIVER_RC_OK;
}

enum ASDriverReturnCode as_driver_api_release_air(enum ASDriverAirLine line) {
    // Ensure the callback pointer was mapped during initialization
    if (as_driver_handler.release_air == NULL) {
        return AS_DRIVER_RC_ERROR;
    }

    // Execute the registered hardware abstraction callback and forward its result
    return as_driver_handler.release_air(line);
}

void as_driver_api_set_mission(enum ASDriverMission mission) {
    as_driver_handler.as_mission = mission;
}

enum ASDriverMission as_driver_api_get_mission(void) {
    return as_driver_handler.as_mission;
}

enum ASDriverReturnCode as_driver_api_set_pressures(const struct ASDriverPressures *new_press) {
    if (new_press == NULL) {
        return AS_DRIVER_RC_ERROR;
    }
    memcpy(&as_driver_handler.pressures, new_press, sizeof(struct ASDriverPressures));
    return AS_DRIVER_RC_OK;
}

enum ASDriverReturnCode as_driver_api_get_pressures(struct ASDriverPressures *out_press) {
    if (out_press == NULL) {
        return AS_DRIVER_RC_ERROR;
    }
    memcpy(out_press, &as_driver_handler.pressures, sizeof(struct ASDriverPressures));
    return AS_DRIVER_RC_OK;
}

enum ASDriverReturnCode as_driver_api_set_mechanical_sensors(const struct ASDriverMechanicalSensors *new_mech) {
    if (new_mech == NULL) {
        return AS_DRIVER_RC_ERROR;
    }
    memcpy(&as_driver_handler.mechanical_sensors, new_mech, sizeof(struct ASDriverMechanicalSensors));
    return AS_DRIVER_RC_OK;
}

enum ASDriverReturnCode as_driver_api_get_mechanical_sensors(struct ASDriverMechanicalSensors *out_mech) {
    if (out_mech == NULL) {
        return AS_DRIVER_RC_ERROR;
    }
    memcpy(out_mech, &as_driver_handler.mechanical_sensors, sizeof(struct ASDriverMechanicalSensors));
    return AS_DRIVER_RC_OK;
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

void as_driver_api_set_watchdog_worked(bool status) {
    as_driver_handler.is_watchdog_worked = status;
}

bool as_driver_api_get_watchdog_worked(void) {
    return as_driver_handler.is_watchdog_worked;
}

void as_driver_api_set_watchdog_check(bool status) {
    as_driver_handler.is_watchdog_check = status;
}

bool as_driver_api_get_watchdog_check(void) {
    return as_driver_handler.is_watchdog_check;
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

void as_driver_api_set_res_go(bool status) {
    as_driver_handler.is_res_go = status;
}

bool as_driver_api_get_res_go(void) {
    return as_driver_handler.is_res_go;
}

void as_driver_api_set_res_emergency(bool status) {
    as_driver_handler.is_res_emergency = status;
}

bool as_driver_api_get_res_emergency(void) {
    return as_driver_handler.is_res_emergency;
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