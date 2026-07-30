/*!
 * \file shutdown-api.h
 * \author Alessandro Bridi <ale.bridi15@gmail.com>
 * \date 2026-07-29
 * \brief This file defines functions for the Shutdown line management.
 */
#ifndef SHUTDOWN_API_H
#define SHUTDOWN_API_H

#include "shutdown.h"

/*!
 * \brief Initializes the shutdown line management using the provided handler.
 *
 * \param[in] control_relay_callback A callback function to set the state of the shutdown line.
 *
 * \retval SHUTDOWN_RC_OK if the shutdown line management was initialized successfully.
 * \retval SHUTDOWN_RC_ERROR if any error occurred during initialization (e.g. NULL callback).
 */
enum ShutdownReturnCode shutdown_api_init(shutdown_control_relay_callback control_relay_callback);

/*!
 * \brief Controls the state of the shutdown line relay.
 *
 * \param[in] relay_state The desired state of the shutdown line (true for closed, false for open).
 *
 * \retval SHUTDOWN_RC_OK if the shutdown line relay state was set successfully.
 * \retval SHUTDOWN_RC_ERROR if an error occurred while setting the shutdown line relay state.
 */
enum ShutdownReturnCode shutdown_api_control_relay(bool relay_state);

/*!
 * \brief Sets the voltage reading for the specified shutdown line position.
 *
 * \param[in] name    The reading position for which to set the shutdown line state.
 * \param[in] voltage The voltage reading to set for the specified reading position.
 *
 * \retval SHUTDOWN_RC_OK if the voltage reading was set successfully.
 * \retval SHUTDOWN_RC_ERROR if an error occurred while setting the voltage reading (e.g. invalid reading position).
 */
enum ShutdownReturnCode shutdown_api_set_voltage(enum ShutdownName name, float voltage);

/*!
 * \brief Gets the current state of the shutdown line for the specified reading position.
 *
 * \param[in] name The reading position for which to get the shutdown line state.
 *
 * \retval SHUTDOWN_STATE_OPEN if the shutdown line is open (voltage below the lower threshold).
 * \retval SHUTDOWN_STATE_CLOSED if the shutdown line is closed (voltage above the upper threshold).
 * \retval SHUTDOWN_STATE_ERROR if the shutdown line is in an implausible state (voltage between the lower and upper thresholds).
 */
enum ShutdownState shutdown_api_get_state(enum ShutdownName name);

/*!
 * \brief Gets the voltage reading for the specified shutdown line position.
 *
 * \param[in] name The reading position for which to get the voltage reading.
 *
 * \return The voltage reading for the specified reading position.
 */
float shutdown_api_get_voltage(enum ShutdownName name);

#endif // SHUTDOWN_API_H
