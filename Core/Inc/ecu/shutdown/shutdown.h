/*!
 * \file shutdown.h
 * \author Alessandro Bridi <ale.bridi15@gmail.com>
 * \date 2026-07-29
 * \brief This file defines types and callbacks for the Shutdown line management.
 */
#ifndef SHUTDOWN_H
#define SHUTDOWN_H

#include <stdint.h>

/*!
 * \brief Return codes for the shutdown_set_state_callback function.
 */
enum ShutdownReturnCode : uint8_t {
    SHUTDOWN_RC_OK,    /*!< The shutdown line state was set successfully. */
    SHUTDOWN_RC_ERROR, /*!< An error occurred while setting the shutdown line state. */
};

/*!
 * \brief Possible reading positions for the shutdown line.
 */
enum ShutdownReading : uint8_t {
    SHUTDOWN_READING_BEFORE_ECU, /*!< The shutdown line is being read before the ECU. */
    SHUTDOWN_READING_AFTER_ECU,  /*!< The shutdown line is being read after the ECU. */
    SHUTDOWN_READING_COUNT       /*!< The number of possible reading positions for the shutdown line. */
};

/*!
 * \brief Possible states for the shutdown line.
 */
enum ShutdownState : uint8_t {
    SHUTDOWN_STATE_OPEN,   /*!< The shutdown line is open. */
    SHUTDOWN_STATE_CLOSED, /*!< The shutdown line is closed. */
    SHUTDOWN_STATE_COUNT   /*!< The number of possible states for the shutdown line. */
};

/*!
 * \brief Callback function type for setting the state of the shutdown line.
 *
 * \param[in] state The desired state of the shutdown line.
 *
 * \retval SHUTDOWN_RC_OK if the shutdown line state was set successfully.
 * \retval SHUTDOWN_RC_ERROR if an error occurred while setting the shutdown line state.
 */
typedef enum ShutdownReturnCode (*shutdown_set_state_callback)(enum ShutdownState state);

struct ShutdownHandler {
    shutdown_set_state_callback set_state; /*!< Callback function to set the state of the shutdown line. */
    enum ShutdownState state_before;       /*!< Current state of the shutdown line before ECU. */
    enum ShutdownState state_after;        /*!< Current state of the shutdown line after ECU. */
};

#endif // SHUTDOWN_H
