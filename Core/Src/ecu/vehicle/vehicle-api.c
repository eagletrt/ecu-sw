/*!
 * \file vehicle-api.c
 * \author Dorijan Di Zepp
 * \brief Encapsulated implementation of the vehicle state handler.
 */

#include "vehicle-api.h"
#include "can-primary-api.h"
#include "can-communication-api.h"
#include "eagletrt.h"

/*!
 * \brief Private module-scope instance handling state data.
 */
EAGLETRT_STATIC struct VehicleHandler vehicle_handler;

enum VehicleReturnCode vehicle_api_init(void) {
    vehicle_handler.ts_on_button_pressed = false;
    vehicle_handler.voltage_higher_than_60v = false;
    return VEHICLE_RC_OK;
}

enum VehicleReturnCode vehicle_api_set_ts_on_button_pressed(bool pressed) {
    vehicle_handler.ts_on_button_pressed = pressed;
    return VEHICLE_RC_OK;
}

bool vehicle_api_get_ts_on_button_pressed(void) {
    return vehicle_handler.ts_on_button_pressed;
}

enum VehicleReturnCode vehicle_api_set_voltage_higher_than_60v(bool is_higher) {
    vehicle_handler.voltage_higher_than_60v = is_higher;
    return VEHICLE_RC_OK;
}

bool vehicle_api_get_voltage_higher_than_60v(void) {
    return vehicle_handler.voltage_higher_than_60v;
}

void vehicle_api_set_ts_state_to_require(bool state) {
    vehicle_handler.ts_state_to_require = state;
}

bool vehicle_api_get_ts_state_to_require(void) {
    return vehicle_handler.ts_state_to_require;
}

void vehicle_api_set_tsac_status(enum CanPrimaryTsacstatusMainboardstatus status) {
    if (status >= CAN_PRIMARY_TSACSTATUS_MAINBOARDSTATUS_TS_ON + 1) {
        return; // Invalid status, do not update
    }
    vehicle_handler.tsac_status = status;
}

enum CanPrimaryTsacstatusMainboardstatus vehicle_api_get_tsac_status(void) {
    return vehicle_handler.tsac_status;
}

void vehicle_api_periodically_require_tsac_status(uint32_t tick) {
    EAGLETRT_STATIC uint32_t last_tick = 0;

    if ((tick - last_tick) >= can_primary_cycle_time_bmsset) {
        last_tick = tick;
        union CanPrimaryMessages msg = { 0 };
        msg.bmsset.status = vehicle_handler.ts_state_to_require;
        struct CanCommunicationFrame frame = { 0 };
        if (can_primary_api_serialize_from_index(CAN_PRIMARY_MESSAGE_INDEX_BMSSET, &msg, frame.data) == 8) {
            frame.id = CAN_PRIMARY_MESSAGE_FRAME_ID_BMSSET;
            frame.length = can_primary_byte_size_bmsset;
            can_communication_api_add_to_tx(CAN_COMMUNICATION_NETWORK_PRIMARY, &frame);
        }
    }
}
