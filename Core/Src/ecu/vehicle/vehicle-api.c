/*!
 * \file vehicle-api.c
 * \author Dorijan Di Zepp
 * \brief Encapsulated implementation of the vehicle state handler.
 */

#include "vehicle-api.h"
#include "eagletrt-api.h"

/*!
 * \brief Private module-scope instance handling state data.
 */
EAGLETRT_STATIC struct VehicleHandler vehicle_handler;

enum VehicleReturnCode vehicle_api_init(void) {
    vehicle_handler.ts_on_requested = false;
    vehicle_handler.voltage_higher_than_60v = false;
    return VEHICLE_RC_OK;
}

enum VehicleReturnCode vehicle_api_set_ts_on_requested(bool requested) {
    vehicle_handler.ts_on_requested = requested;
    return VEHICLE_RC_OK;
}

bool vehicle_api_get_ts_on_requested(void) {
    return vehicle_handler.ts_on_requested;
}

enum VehicleReturnCode vehicle_api_set_voltage_higher_than_60v(bool is_higher) {
    vehicle_handler.voltage_higher_than_60v = is_higher;
    return VEHICLE_RC_OK;
}

bool vehicle_api_get_voltage_higher_than_60v(void) {
    return vehicle_handler.voltage_higher_than_60v;
}
