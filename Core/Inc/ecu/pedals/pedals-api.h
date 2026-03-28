/*!
 * \file pedals-api.h
 * \author Dorijan Di Zepp
 * \date 2026-03-25
 * \brief This module provides the APIs for the pedals module
 * \note All percentages are normalized (0.0f to 1.0f). 
 */

#ifndef PEDALS_API_H
#define PEDALS_API_H

#include "pedals.h"

/*!
 * \brief Initializes the pedals internal handler and resets all percentages to zero.
 * \return PEDALS_RC_OK on successful initialization.
 */
enum PedalsReturnCode pedals_api_init();

/*!
 * \brief Sets the current throttle percentage.
 * \note The value will remain unchanged if the input is out of range (0.0f to 1.0f).
 * \param[in] throttle The throttle position in percentage (0.0f to 1.0f).
 * \retval PEDALS_RC_OK on success.
 * \retval PEDALS_RC_ERROR if the value is out of range or system is unavailable.
 */
enum PedalsReturnCode pedals_api_set_throttle(float throttle);

/*!
 * \brief Sets the current brake percentage.
 * \note The value will remain unchanged if the input is out of range (0.0f to 1.0f).
 * \param[in] brake The brake position in percentage (0.0f to 1.0f).
 * \retval PEDALS_RC_OK on success.
 * \retval PEDALS_RC_ERROR if the value is out of range or system is unavailable.
 */
enum PedalsReturnCode pedals_api_set_brake(float brake);

/*!
 * \brief Sets the physical brake pressure.
 * \note The value will remain unchanged if the input is negative.
 * \param[in] brake_pressure The physical brake pressure value in bars.
 * \retval PEDALS_RC_OK on success.
 * \retval PEDALS_RC_ERROR if the value is invalid or system is unavailable.
 */
enum PedalsReturnCode pedals_api_set_brake_pressure(float brake_pressure);

/*!
 * \brief Calculates the driver's requested motor torque based on throttle position.
 * \details Maps throttle (0.0 to 1.0) to torque (0 to \c PEDALS_MAX_TORQUE).
 * \return The calculated torque in Nm. 
 * \note Returns 0.0 if the pedals module is uninitialized.
 */
float pedals_api_get_requested_throttle_torque();

/*!
 * \brief Determines if the brake pedal is sufficiently pressed for stationary holding.
 * \details Compares current brake values against the predefined safety threshold.
 * \return \c true if the brake pressure is above the safety threshold, \c false otherwise.
 */
bool pedals_api_is_brake_pressed();

/*!
 * \brief Retrieves the latest valid throttle percentage.
 * \return The throttle percentage (0.0 - 1.0).
 */
float pedals_api_get_throttle();

/*!
 * \brief Retrieves the latest valid brake percentage.
 * \return The brake percentage (0.0 - 1.0).
 */
float pedals_api_get_brake();

/*!
 * \brief Retrieves the latest valid brake pressure.
 * \return The pressure in bars (0.0f - \c PEDALS_MAX_BRAKE_PRESSURE_BAR).
 */
float pedals_api_get_brake_pressure();

#endif