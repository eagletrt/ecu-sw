/*!
 * \file tractive-system-api.h
 * \author Dorijan Di Zepp
 * \date 2026-03-22
 * \brief Public API for the Tractive System (TS) control module.
 */

#ifndef TRACTIVE_SYSTEM_API_H
#define TRACTIVE_SYSTEM_API_H

#include "tractive-system.h"

/*!
 * \brief Initializes the TS handler and internal variables.
 * \param[in] send_ts_command Hardware-level callback for issuing TS commands.
 * \retval TS_RC_OK if initialization succeeded
 * \retval TS_RC_ERROR if the callback is NULL.
 */
enum TSReturnCode ts_api_init(ts_command_callback send_ts_command);

/*!
 * \brief Sends a physical command to the hardware.
 * \param[in] command An enum indicting if the TS has to be turned ON or OFF
 * \details To be used inside FSM transition functions like 'start_ts_precharge'.
 * \retval TS_RC_OK if the request has been received
 * \retval TS_RC_ERROR otherwise
 */
enum TSReturnCode ts_api_request_command(enum TSCommand command);

#endif