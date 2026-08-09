#include "tsac-api.h"
#include "can-primary-api.h"
#include "can-communication-api.h"
#include "logger-api.h"

EAGLETRT_STATIC struct TsacHandler tsac_handler;

enum TsacReturnCode tsac_api_init(tsac_get_tick_callback get_tick) {
    if (get_tick == NULL) {
        return TSAC_RC_NULL_POINTER;
    }
    tsac_handler.get_tick = get_tick;
    tsac_handler.voltage_higher_than_60v = false;
    tsac_handler.ts_state_to_require = false;
    return TSAC_RC_OK;
}

enum TsacReturnCode tsac_api_set_voltage_higher_than_60v(bool is_higher) {
    tsac_handler.voltage_higher_than_60v = is_higher;
    return TSAC_RC_OK;
}

bool tsac_api_get_voltage_higher_than_60v(void) {
    return tsac_handler.voltage_higher_than_60v;
}

enum TsacReturnCode tsac_api_set_ts_state_to_require(bool state) {
    tsac_handler.ts_state_to_require = state;
    return TSAC_RC_OK;
}

bool tsac_api_get_ts_state_to_require(void) {
    return tsac_handler.ts_state_to_require;
}

enum TsacReturnCode tsac_api_set_tsac_status(enum CanPrimaryTsacstatusMainboardstatus status) {
    if (status >= CAN_PRIMARY_TSACSTATUS_MAINBOARDSTATUS_TS_ON + 1) {
        return TSAC_RC_ERROR; // Invalid status, do not update
    }
    if (tsac_handler.get_tick != NULL) {
        tsac_handler.last_status_received_tick = tsac_handler.get_tick();
    }
    tsac_handler.tsac_status = status;
    return TSAC_RC_OK;
}

enum CanPrimaryTsacstatusMainboardstatus tsac_api_get_tsac_status(void) {
    return tsac_handler.tsac_status;
}

void tsac_api_periodically_require_tsac_status(uint32_t tick) {
    EAGLETRT_STATIC uint32_t last_tick = 0;

    if ((tick - last_tick) >= can_primary_cycle_time_bmsset) {
        last_tick = tick;
        union CanPrimaryMessages msg = { 0 };
        msg.bmsset.status = tsac_handler.ts_state_to_require;
        struct CanCommunicationFrame frame = { 0 };
        if (can_primary_api_serialize_from_id(CAN_PRIMARY_MESSAGE_FRAME_ID_BMSSET, &msg, frame.data) >= 0) {
            frame.id = CAN_PRIMARY_MESSAGE_FRAME_ID_BMSSET;
            frame.length = can_primary_byte_size_bmsset;
            can_communication_api_add_to_tx(CAN_COMMUNICATION_NETWORK_PRIMARY, &frame);
        }
    }
}

bool tsac_api_is_tsac_status_timeout(void) {
    if (tsac_handler.get_tick == NULL) {
        return true; // If get_tick is not set, consider it timed out
    }
    uint32_t current_tick = tsac_handler.get_tick();
    return (current_tick - tsac_handler.last_status_received_tick) > TSAC_TIMEOUT_MS;
}
