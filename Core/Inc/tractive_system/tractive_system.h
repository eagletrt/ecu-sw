/*!
 * \file tractive_system.h
 * \author Dorijan Di Zepp
 * \date 15-02-2026
 * \brief Logic module for managing the Tractive System (TS)
 */

#ifndef TRACTIVE_SYSTEM_H
#define TRACTIVE_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>

/*!
 * \brief Return codes for the tractive system module APIs.
 */
enum TSReturnCode {
    TS_RC_OK,   /*!< Operation completed successfully */
    TS_RC_ERROR /*!< Operation NOT completed successfully */
};

/*!
 * \brief TS commands.
 */
enum TSCommand {
    TS_COMMAND_ON, /*!< Send a logical ON */
    TS_COMMAND_OFF /*!< Send a logical OFF */
};

/*!
 * \brief TS state possibilities.
 */
enum TsStatus {
    TS_STATUS_OFF,
    TS_STATUS_PRECHARGE,
    TS_STATUS_ON,
    TS_STATUS_FATAL,
    TS_STATUS_UNKNOWN
};

/*!
 * \brief Internal state of the tractive system module.
 */
struct TsHandler {
    /*!< \brief Function to physically send ts commands */
    enum TSReturnCode (*send_ts_command)(enum TSCommand);

    /*!<
     * \brief Current physical status of the TS.
     * \details Represents the last confirmed state received from the CAN bus 
     */
    enum TsStatus status;

    /*!<
     * \brief Pending "Power On" request flag.
     * \details Set to true when the driver initiates the start sequence. 
     * Automatically cleared once the status transition to TS_STATUS_ON is confirmed.
     */
    bool request_on;

    /*!<
     * \brief Pending "Power Off" request flag.
     * \details Set to true when a shutdown is commanded. 
     * Automatically cleared once the status transition to TS_STATUS_OFF is confirmed.
     */
    bool request_off;
};

/*!
 * \brief Initializes the TS handler and internal variables.
 * \return TS_RC_OK if the initialization completed, TS_RC_ERROR in case the callback is a NULL
 */
enum TSReturnCode TS_init(enum TSReturnCode (*send_ts_command)(enum TSCommand));

/*!
 * \brief Set the status of the Tractive System.
 * \param TsStatus The current TS status
 * \return TS_RC_OK if the status is possible otherwise TS_RC_ERROR
 */
enum TSReturnCode TS_set_status(enum TsStatus);

/*!
 * \brief Returns the current filtered status of the Tractive System.
 * \return The current status of the TS
 */
enum TsStatus TS_get_status();

/*!
 * \brief Utility function to get the string representation of a state.
 * \param status The status of which the name is wanted
 * \return The name of the state as a char pointer
 */
const char *TS_get_state_name(enum TsStatus status);

/*!
 * \brief Set a request to transition to TS_ON.
 * \return TS_RC_OK if the request has been received, TS_RC_ERROR otherwise
 */
enum TSReturnCode TS_request_power_on();

/*!
 * \brief Set a request to transition to TS_OFF.
 * \return * \return TS_RC_OK if the request has been received, TS_RC_ERROR otherwise
 */
enum TSReturnCode TS_request_power_off();

/*!
 * \brief Checks if a Power On request is pending.
 * \return true is a power on request has been sent correclty previously, false otherwise
 */
bool TS_is_power_on_requested();

/*!
 * \brief Checks if a Power Off request is pending.
 * \return true is a power off request has been sent correclty previously, false otherwise
 */
bool TS_is_power_off_requested();

/*!
 * \brief Clears (resets) all pending requests.
 * \return TS_RC_OK
 */
enum TSReturnCode TS_clear_request();

#endif