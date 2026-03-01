/*!
 * \file pedals.h
 * \author Dorijan Di Zepp
 * \date 26-02-2026
 * \brief Module for managing the pedals states.
 */

#ifndef PEDALS_H
#define PEDALS_H

#include <stdint.h>
#include <stdbool.h>

#define PEDALS_MAX_BRAKE_PRESSURE_BAR (100.0f)
#define PEDALS_MAX_TORQUE_NM (91.0f)
#define PEDALS_BRAKE_THRESHOLD_PERCENTAGE (0.05f)

/*!
 * \brief Return codes for the pedals module APIs.
 */
enum PedalsReturnCode {
    PEDALS_RC_OK,   /*!< Operation completed successfully */
    PEDALS_RC_ERROR /*!< Operation NOt completed */
};

/*!
 * \brief Internal state of the pedals module.
 */
struct PedalsHandler {
    float throttle;       /*!< The throttle position in percentage from 0.0 to 1.0 */
    float brake;          /*!< The brake position in percentage from 0.0 to 1.0 */
    float brake_pressure; /*!< Brake pressure in bars */
    bool is_available;    /*!< Health status of the pedal board connection */
};

#endif