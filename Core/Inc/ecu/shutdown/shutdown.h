/*!
 * \file shutdown.h
 * \author Alessandro Bridi <ale.bridi15@gmail.com>
 * \date 2026-07-29
 * \brief This file defines types and callbacks for the Shutdown line management.
 */
#ifndef SHUTDOWN_H
#define SHUTDOWN_H

#include <stdint.h>

#define SHUTDOWN_VOLTAGE_THRESHOLD_UPPER (16.0F)
#define SHUTDOWN_VOLTAGE_THRESHOLD_LOWER (0.5F)

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
enum ShutdownName : uint8_t {
    SHUTDOWN_NAME_BEFORE_ECU, /*!< The shutdown line is being read before the ECU. */
    SHUTDOWN_NAME_AFTER_ECU,  /*!< The shutdown line is being read after the ECU. */
    SHUTDOWN_NAME_COUNT       /*!< The number of possible reading positions for the shutdown line. */
};

/*!
 * \brief Possible states for the shutdown line.
 */
enum ShutdownState : uint8_t {
    SHUTDOWN_STATE_OPEN,   /*!< The shutdown line is open. */
    SHUTDOWN_STATE_CLOSED, /*!< The shutdown line is closed. */
    SHUTDOWN_STATE_ERROR,  /*!< The shutdown line is an implausible state. */
    SHUTDOWN_STATE_COUNT   /*!< The number of possible states for the shutdown line. */
};

/*!
 * \brief Callback function type for setting the state of the shutdown line.
 *
 * \param[in] relay_state The desired state of the shutdown line (true for closed, false for open).
 *
 * \retval SHUTDOWN_RC_OK if the shutdown line state was set successfully.
 * \retval SHUTDOWN_RC_ERROR if an error occurred while setting the shutdown line state.
 */
typedef enum ShutdownReturnCode (*shutdown_control_relay_callback)(bool relay_state);

/*!
 * \brief Handler that hold status of the shutdown line.
 */
struct ShutdownHandler {
    shutdown_control_relay_callback control_relay; /*!< Callback function to set the state of the shutdown line. */
    float voltages[SHUTDOWN_NAME_COUNT];           /*!< Array to hold the voltage readings for each shutdown line position. */
};

#endif // SHUTDOWN_H
