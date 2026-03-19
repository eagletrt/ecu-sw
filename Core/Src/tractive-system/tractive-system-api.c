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

enum TSReturnCode ts_api_init(ts_command_callback send_ts_command) {
    if (send_ts_command == NULL)
        return TS_RC_ERROR;

    ts_handler.send_ts_command = send_ts_command;
    return TS_RC_OK;
}

enum TSReturnCode ts_api_request_command(enum TSCommand command) {
    if (ts_handler.send_ts_command != NULL) {
        return ts_handler.send_ts_command(command);
    }
    return TS_RC_ERROR;
}

enum TSReturnCode ts_api_deinit() {
    ts_handler.send_ts_command = NULL;
    return TS_RC_OK;
}