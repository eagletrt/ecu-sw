/*!
 * \file raspberry-api.c
 * \author Dorijan Di Zepp
 * \date 2026-05-03
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

enum RaspberryReturnCode rasp_api_init(raspberry_pin_control_callback pin_control, enum RaspberryControlPinState initial_state) {
    // default to an indeterminate state. we cannot assume hardware alignment
    // until the first successful callback execution
    raspberry_handler.current_pin_state = RASPBERRY_CONTROL_PIN_STATE_UNKNOWN;

    if (pin_control == NULL) {
        return RASPBERRY_RC_ERROR;
    }

    raspberry_handler.pin_control = pin_control; // set the callback
    return rasp_api_change_pin_state(initial_state);
}

enum RaspberryReturnCode rasp_api_change_pin_state(enum RaspberryControlPinState pin_state) {
    if (raspberry_handler.pin_control == NULL ||
        !(pin_state >= 0 && pin_state < RASPBERRY_CONTROL_PIN_STATE_COUNT)) {
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
    }

    // the pin state is known and already set
    raspberry_handler.current_pin_state = pin_state;

    return RASPBERRY_RC_OK;
}

enum RaspberryControlPinState rasp_api_get_pin_state() {
    return raspberry_handler.current_pin_state;
}