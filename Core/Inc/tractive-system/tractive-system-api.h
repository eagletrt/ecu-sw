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
 * \param ts_status Hardware-level callback for retrieving the TS current status.
 * \return TS_RC_OK if initialization succeeded, TS_RC_ERROR if the callback is NULL.
 */
enum TSReturnCode ts_api_init(ts_command_callback send_ts_command, ts_feedback_callback ts_status);

/*!
 * \brief Sends a physical command to the hardware.
 * \param command An enum indicting if the TS has to be turned ON or OFF
 * \details To be used inside FSM transition functions like 'start_ts_precharge'.
 * \return TS_RC_OK if the request has been received, TS_RC_ERROR otherwise
 */
enum TSReturnCode ts_api_request_command(enum TSCommand command);

/*!
 * \brief Returns the current status of the Tractive System.
 * \return The current status of the TS
 */
enum TsStatus ts_api_get_status();

/*!
 * \brief Resets the handler's callback to NULL.
 * \return TS_RC_OK after the callbacks are set to NULL.
 */
enum TSReturnCode ts_api_reset();

#endif