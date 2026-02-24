/*!
 * \file tractive_system.c
 * \author Dorijan Di Zepp
 * \date 15-02-2026
 * \brief Implementation of Tractive System (TS) state tracking.
 */

#include "tractive_system.h"
#include "eagletrt-api.h"

/*!
 * \brief Global string array for debugging status names.
 */
const char *TS_state_names[] = { "OFF", "PRECHARGE", "ON", "FATAL", "UNKNOWN" };

EAGLETRT_STATIC struct TsHandler ts_handler;

enum TSReturnCode TS_init(enum TSReturnCode (*send_ts_command)(enum TSCommand)) {
    if (send_ts_command == NULL)
        return TS_RC_ERROR;

    ts_handler.send_ts_command = send_ts_command;
    ts_handler.status = TS_STATUS_UNKNOWN;
    ts_handler.request_on = false;
    ts_handler.request_off = false;
    return TS_RC_OK;
}

enum TSReturnCode TS_set_status(enum TsStatus status) {
    if (status > TS_STATUS_FATAL)
        return TS_RC_ERROR;

    ts_handler.status = status;
    return TS_RC_OK;
}

enum TsStatus TS_get_status() {
    return ts_handler.status;
}

const char *TS_get_state_name(enum TsStatus status) {
    if (status > TS_STATUS_FATAL)
        return TS_state_names[4];
    return TS_state_names[status];
}

enum TSReturnCode TS_request_power_on() {
    // only allow a request if we are currently OFF or PRECHARGING
    if (ts_handler.status == TS_STATUS_OFF || ts_handler.status == TS_STATUS_PRECHARGE) {
        // if the sending is not ok, abort
        if (ts_handler.send_ts_command(TS_COMMAND_ON) != TS_RC_OK)
            return TS_RC_ERROR;

        ts_handler.request_on = true;
        ts_handler.request_off = false;

        return TS_RC_OK;
    }

    return TS_RC_OK;
}

enum TSReturnCode TS_request_power_off() {
    // power OFF is allowed from any state for safety
    // if the sending is not ok, abort
    if (ts_handler.send_ts_command(TS_COMMAND_OFF) != TS_RC_OK)
        return TS_RC_ERROR;
    ts_handler.request_off = true;
    ts_handler.request_on = false;

    return TS_RC_OK;
}

bool TS_is_power_on_requested() {
    return ts_handler.request_on;
}

bool TS_is_power_off_requested() {
    return ts_handler.request_off;
}

enum TSReturnCode TS_clear_request() {
    ts_handler.request_on = false;
    ts_handler.request_off = false;
    return TS_RC_OK;
}