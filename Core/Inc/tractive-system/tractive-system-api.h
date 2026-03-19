/*!
 * \file tractive-system-api.h
 * \author Dorijan Di Zepp
 * \date 01-03-2026
 * \brief File defining the TS modules' API.
 */

#ifndef TRACTIVE_SYSTEM_API_H
#define TRACTIVE_SYSTEM_API_H

#include "tractive-system.h"
#include "ecu_fsm.h"
#include <stdint.h>
#include <stdbool.h>

/*!
 * \brief Initializes the TS handler and internal variables.
 * \param send_ts_command Hardware-level callback for issuing TS commands.
 * \return TS_RC_OK if initialization succeeded, TS_RC_ERROR if the callback is NULL.
 */
enum TSReturnCode ts_api_init(ts_command_callback send_ts_command);

/*!
 * \brief Sends a physical command to the hardware.
 * \param command An enum indicting if the TS has to be turned ON or OFF
 * \details To be used inside FSM transition functions like 'start_ts_precharge'.
 * \return TS_RC_OK if the request has been received, TS_RC_ERROR otherwise
 */
enum TSReturnCode ts_api_request_command(enum TSCommand command);

/*!
 * \brief Resets the handler's callback to NULL.
 * \return TS_RC_OK after the callbacks are set to NULL.
 */
enum TSReturnCode ts_api_deinit();

#endif