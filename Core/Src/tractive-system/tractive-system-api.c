/*!
 * \file tractive-system-api.c
 * \author Dorijan Di Zepp
 * \date 01-03-2026
 * \brief Implementation of Tractive System (TS) module.
 * \details Provides am interface for sending activation/deactivation 
 * commands to the TS hardware and fetching its status through 
 * dedicated callback functions.
 */

#include "tractive-system-api.h"
#include "eagletrt-api.h"

EAGLETRT_STATIC struct TsHandler ts_handler;

enum TSReturnCode ts_api_init(ts_command_callback send_ts_command, ts_feedback_callback ts_status) {
    if (send_ts_command == NULL || ts_status == NULL)
        return TS_RC_ERROR;

    ts_handler.send_ts_command = send_ts_command;
    ts_handler.ts_status = ts_status;
    return TS_RC_OK;
}

enum TSReturnCode ts_api_request_command(enum TSCommand command) {
    if (ts_handler.send_ts_command != NULL) {
        if (ts_handler.send_ts_command(command) != TS_RC_OK)
            return TS_RC_ERROR;

        return TS_RC_OK;
    }
    return TS_RC_ERROR;
}

enum TsStatus ts_api_get_status() {
    if (ts_handler.ts_status == NULL)
        return TS_STATUS_UNKNOWN;
    return ts_handler.ts_status();
}

enum TSReturnCode ts_api_reset() {
    ts_handler.send_ts_command = NULL;
    ts_handler.ts_status = NULL;
    return TS_RC_OK;
}