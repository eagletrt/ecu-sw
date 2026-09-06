/*!
 * \file vehicle-api.c
 * \author Dorijan Di Zepp
 * \brief Encapsulated implementation of the vehicle state handler.
 */

#include "vehicle-api.h"
#include <stddef.h>
#include "eagletrt.h"

/*!
 * \brief Private module-scope instance handling state data.
 */
EAGLETRT_STATIC struct VehicleHandler vehicle_handler;

enum VehicleReturnCode vehicle_api_init(vehicle_ptt_control_callback ptt_control) {
    if (ptt_control == NULL) {
        return VEHICLE_RC_NULL_POINTER;
    }
    vehicle_handler.ptt_control = ptt_control;
    vehicle_handler.ts_on_button_pressed = false;
    return VEHICLE_RC_OK;
}

enum VehicleReturnCode vehicle_api_set_ts_on_button_pressed(bool pressed) {
    vehicle_handler.ts_on_button_pressed = pressed;
    return VEHICLE_RC_OK;
}

bool vehicle_api_get_ts_on_button_pressed(void) {
    return vehicle_handler.ts_on_button_pressed;
}

enum VehicleReturnCode vehicle_api_set_ptt(bool state) {
    if (vehicle_handler.ptt_control == NULL) {
        return VEHICLE_RC_ERROR;
    }
    return vehicle_handler.ptt_control(state);
}
