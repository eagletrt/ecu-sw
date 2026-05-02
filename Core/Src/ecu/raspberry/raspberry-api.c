/*!
 * \file raspberry-api.c
 * \author Dorijan Di Zepp
 * \date 2026-05-02
 * \brief Hardware-agnostic module for raspberry control logic.
 * 
 * This module mangaes both startup and shutdown of the raspberry board.
 */

#include <string.h>
#include "raspberry-api.h"
#include "eagletrt-api.h"

/*!
 * \brief Static raspberry handler
 */
EAGLETRT_STATIC struct RaspberryHandler raspberry_handler;

/*!
 * \brief Validates that the provided pin state is within the allowed range
 * \param[in] pin_state The enum value representing the target pin state
 * \return true if the state is valid and defined; false otherwise
 */
EAGLETRT_STATIC_INLINE bool raspberry_prv_is_control_pin_state_type_valid(enum RaspberryControlPinState pin_state) {
    // verify that the buzzer type is valid and it doesn't exceed the handler size
    // casting to uint32_t makes negative values wrap to huge positive values
    return ((uint32_t)pin_state < (uint32_t)RASPBERRY_CONTROL_PIN_STATE_COUNT);
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
EAGLETRT_STATIC_INLINE enum RaspberryReturnCode raspberry_prv_call_pin_control_and_set_current_state(enum RaspberryControlPinState pin_state) {
    if (raspberry_handler.pin_control == NULL ||
        !raspberry_prv_is_control_pin_state_type_valid(pin_state)) {
        // leave as it is the current pin state as the callback
        // has been "ignored"
        return RASPBERRY_RC_ERROR;
    }

    // invoke the callback with the requested pin state
    enum RaspberryReturnCode callback_rc = raspberry_handler.pin_control(pin_state);

    if (callback_rc != RASPBERRY_RC_OK) {
        // we are not sure about the pin state
        // as the callback failed and it might have changed the state
        raspberry_handler.current_pin_state = RASPBERRY_CONTROL_PIN_STATE_UNKNOWN;
        return RASPBERRY_RC_ERROR;
    } else {
        // the pin state is known and already set
        raspberry_handler.current_pin_state = pin_state;
    }

    return RASPBERRY_RC_OK;
}

enum RaspberryReturnCode rasp_api_init(raspberry_pin_control_callback pin_control, enum RaspberryControlPinState initial_state) {
    // default to an indeterminate state. we cannot assume hardware alignment
    // until the first successful callback execution
    raspberry_handler.current_pin_state = RASPBERRY_CONTROL_PIN_STATE_UNKNOWN;

    if (pin_control == NULL)
        return RASPBERRY_RC_ERROR;

    raspberry_handler.pin_control = pin_control; // set the callback
    return raspberry_prv_call_pin_control_and_set_current_state(initial_state);
}

enum RaspberryReturnCode rasp_api_change_pin_state(enum RaspberryControlPinState pin_state) {
    return raspberry_prv_call_pin_control_and_set_current_state(pin_state);
}

enum RaspberryControlPinState rasp_api_get_pin_state() {
    return raspberry_handler.current_pin_state;
}