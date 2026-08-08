/*!
 * \file vehicle-api.c
 * \author Dorijan Di Zepp
 * \brief Encapsulated implementation of the vehicle state handler.
 */

#include "vehicle-api.h"
#include <stdio.h>
#include "eagletrt.h"

/*!
 * \brief Private module-scope instance handling state data.
 */
EAGLETRT_STATIC struct VehicleHandler vehicle_handler;

enum VehicleReturnCode vehicle_api_init(vehicle_tson_pressed_callback ts_on_get_button_pressed) {
    if (ts_on_get_button_pressed == NULL) {
        return VEHICLE_RC_NULL_POINTER;
    }
    vehicle_handler.ts_on_get_button_pressed = ts_on_get_button_pressed;
    vehicle_handler.ts_on_button_pressed = false;
    return VEHICLE_RC_OK;
}

enum VehicleReturnCode vehicle_api_set_ts_on_button_pressed(bool pressed) {
    vehicle_handler.ts_on_button_pressed = pressed;
    return VEHICLE_RC_OK;
}

bool vehicle_api_get_ts_on_button_pressed(void) {
    if (vehicle_handler.ts_on_get_button_pressed) {
        return vehicle_handler.ts_on_get_button_pressed();
    }
    return false;
}
