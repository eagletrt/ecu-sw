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
 * \param[in] set_state_callback A callback function to set the state of the shutdown line.
 *
 * \retval SHUTDOWN_RC_OK if the shutdown line management was initialized successfully.
 * \retval SHUTDOWN_RC_ERROR if any error occurred during initialization.
 */
enum ShutdownReturnCode shutdown_api_init(shutdown_set_state_callback set_state_callback);

/*!
 * \brief Sets the state of the shutdown line using the provided state.
 *
 * \param[in] state The desired state of the shutdown line.
 *
 * \retval SHUTDOWN_RC_OK if the shutdown line state was set successfully.
 * \retval SHUTDOWN_RC_ERROR if an error occurred while setting the shutdown line state.
 */
enum ShutdownReturnCode shutdown_api_set_state(enum ShutdownState state);

/*!
 * \brief Sets the state of the shutdown line for a specific reading position.
 *
 * \param[in] reading The reading position for which to set the shutdown line state.
 * \param[in] state The desired state of the shutdown line.
 *
 * \retval SHUTDOWN_RC_OK if the shutdown line state was set successfully for the specified reading position.
 * \retval SHUTDOWN_RC_ERROR if an error occurred while setting the shutdown line state for
 */
enum ShutdownReturnCode shutdown_api_set_reding_state(enum ShutdownReading reading, enum ShutdownState state);

/*!
 * \brief Gets the current state of the shutdown line based on the specified reading position.
 *
 * \param[in] reading The reading position for which to get the shutdown line state.
 *
 * \return The current state of the shutdown line at the specified reading position.
 */
enum ShutdownState shutdown_api_get_reading_state(enum ShutdownReading reading);

#endif // SHUTDOWN_API_H
