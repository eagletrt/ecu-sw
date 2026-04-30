/*!
 * \file rasp-api.c
 * \author Dorijan Di Zepp
 * \date 2026-04-28
 * \brief Hardware-agnostic module for raspberry control logic.
 * 
 * This module mangaes both startup and shutdown of the raspberry board.
 */

#include <string.h>
#include "rasp-api.h"
#include "eagletrt-api.h"

/*!
 * \brief Static raspberry handler
 */
EAGLETRT_STATIC struct RaspHandler rasp_handler;

/*!
 * \brief Validates that the provided pin state is within the allowed range
 * \param[in] pin_state The enum value representing the target pin state
 * \return true if the state is valid and defined; false otherwise
 */
EAGLETRT_STATIC_INLINE bool prv_is_control_pin_state_type_valid(enum RaspControlPinState pin_state) {
    // verify that the buzzer type is valid and it doesn't exceed the handler size
    // casting to uint32_t makes negative values wrap to huge positive values
    return ((uint32_t)pin_state < (uint32_t)RASP_CONTROL_PIN_STATE_COUNT);
}

/*!
 * \brief Internal helper to execute the hardware callback and synchronize the local state
 * \details This function acts as the bridge between software intent and hardware reality. 
 * If the hardware layer (callback) reports a failure, the local state is invalidated 
 * (set to UNKNOWN) to prevent the ECU from making logic decisions based on stale or 
 * incorrect pin data
 * \param[in] pin_state The target state to be physically applied to the pin
 * \retval RASP_RC_OK if the hardware confirmed the state change
 * \retval RASP_RC_ERROR if the hardware was not able to change the state
 */
EAGLETRT_STATIC_INLINE enum RaspReturnCode prv_call_pin_control_and_set_current_state(enum RaspControlPinState pin_state) {
    if (rasp_handler.pin_control == NULL ||
        !prv_is_control_pin_state_type_valid(pin_state)) {
        // leave as it is the current pin state as the callback
        // has been "ignored"
        return RASP_RC_ERROR;
    }

    // invoke the callback with the requested pin state
    enum RaspReturnCode callback_rc = rasp_handler.pin_control(pin_state);

    if (callback_rc != RASP_RC_OK) {
        // we are not sure about the pin state
        // as the callback failed and it might have changed the state
        rasp_handler.current_pin_state = RASP_CONTROL_PIN_STATE_UNKNOWN;
        return RASP_RC_ERROR;
    } else {
        // the pin state is known and already set
        rasp_handler.current_pin_state = pin_state;
    }

    return RASP_RC_OK;
}

enum RaspReturnCode rasp_api_init(rasp_pin_control_callback pin_control, enum RaspControlPinState initial_state) {
    // default to an indeterminate state. we cannot assume hardware alignment
    // until the first successful callback execution
    rasp_handler.current_pin_state = RASP_CONTROL_PIN_STATE_UNKNOWN;

    if (pin_control == NULL)
        return RASP_RC_ERROR;

    rasp_handler.pin_control = pin_control; // set the callback
    return prv_call_pin_control_and_set_current_state(initial_state);
}

enum RaspReturnCode rasp_api_change_pin_state(enum RaspControlPinState pin_state) {
    return prv_call_pin_control_and_set_current_state(pin_state);
}

enum RaspControlPinState rasp_api_get_pin_state() {
    return rasp_handler.current_pin_state;
}