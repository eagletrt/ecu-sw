/*!
 * \file tractive-system-api.c
 * \author Dorijan Di Zepp
 * \date 2026-03-22
 * \brief Implementation of Tractive System (TS) module.
 * \details Provides an interface for sending activation/deactivation 
 * commands to the TS hardware through dedicated callback function.
 */

#include "tractive-system-api.h"
#include "eagletrt-api.h"

/*!
 * \brief Internal module handler.
 * \details Hidden from external linkage to enforce API-only access.
 */
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