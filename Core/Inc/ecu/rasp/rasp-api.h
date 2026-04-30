/*!
 * \file rasp-api.h
 * \author Dorijan Di Zepp
 * \date 2026-04-28
 * \brief This file defines the raspberry module's API to operate on the control pin to control
 * both startup and shutdown procedures of the raspberry board.
 */

#ifndef RASP_API_H
#define RASP_API_H

#include "rasp.h"

/*!
 * \brief Initializes the raspberry handler and sets the initial hardware pin state
 * \param[in] pin_control Callback function to manage the physical pin state
 * \param[in] initial_state The desired hardware state to be set upon initialization
 * \retval RASP_RC_OK If the initialization and hardware setup were successful
 * \retval RASP_RC_ERROR If the callback is \c NULL or the hardware failed to respond
 */
enum RaspReturnCode rasp_api_init(rasp_pin_control_callback pin_control, enum RaspControlPinState initial_state);

/*!
 * \brief Requests a change in the control pin state to trigger a startup or shutdown sequence
 * \param[in] pin_state The target state to transition the control pin to
 * \retval RASP_RC_OK If the pin state was successfully updated and verified
 * \retval RASP_RC_ERROR If the state transition failed or an invalid state was requested
 */

/*!
 * \brief Requests a change in the control pin state to trigger a startup or shutdown sequence
 * \param[in] pin_state The target state to transition the control pin to
 * \retval RASP_RC_OK If the pin state was successfully updated and verified
 * \retval RASP_RC_ERROR If the state transition failed or an invalid state was requested
 * \warning In the event of a callback failure, the internal state tracker 
 * is set to \c RASP_CONTROL_PIN_STATE_UNKNOWN. This design choice prevents the system from 
 * assuming a hardware state that cannot be verified
 * \note An UNKNOWN state should be handled by the caller as a hardware-in-transition 
 * or a fault. It is recommended to re-attempt the state change or perform a 
 * resynchronization to return the system to a deterministic state (ON or OFF).
 */
enum RaspReturnCode rasp_api_change_pin_state(enum RaspControlPinState pin_state);

/*!
 * \brief Retrieves the current known state of the raspberry control pin
 * \return The current \ref RaspControlPinState value
 */
enum RaspControlPinState rasp_api_get_pin_state();

#endif