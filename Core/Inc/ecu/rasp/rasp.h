/*!
 * \file rasp.h
 * \author Dorijan Di Zepp
 * \date 2026-04-28
 * \brief This file defines the structures to manage the startup and shutdown 
 * procedure of the raspberry pi to prevent filesystem corruption
 */

#ifndef RASP_H
#define RASP_H

#include <stdbool.h>

/*!
 * \brief Return codes for the raspberry module APIs
 */
enum RaspReturnCode {
    RASP_RC_OK,   /*!< Operation completed successfully */
    RASP_RC_ERROR /*!< The requested operation failed or encountered an invalid state */
};

/*!
 * \brief Enum values to control the pin state linked to the raspberry
 */
enum RaspControlPinState {
    RASP_CONTROL_PIN_STATE_UNKNOWN, /*!< Indeterminate state due to error or initialization. */
    RASP_CONTROL_PIN_STATE_OFF,     /*!< Pin state that allows the raspberry to remain powered/running. */
    RASP_CONTROL_PIN_STATE_ON,      /*!< Pin state that triggers the raspberry shutdown/power-off. */
    RASP_CONTROL_PIN_STATE_COUNT,   /*!< Sentinel for range validation. */
};

/*!
 * \brief Signature for controling the pin state linked to the raspberry
 * \param pin_state The state to be set to the corresponding pin
 * \retval RASP_RC_OK Pin state changed successfully
 * \retval RASP_RC_ERROR Pin state unaltered
 */
typedef enum RaspReturnCode (*rasp_pin_control_callback)(enum RaspControlPinState pin_state);

/*!
 * \brief Handler for the raspberry control module.
 */
struct RaspHandler {
    rasp_pin_control_callback pin_control; /*!< Callback to control the raspberry's pin state */

    enum RaspControlPinState current_pin_state; /*!< Current state of raspberry's pin */
};

#endif