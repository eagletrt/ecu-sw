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
 * \brief TS commands.
 */
enum TSCommand {
    TS_COMMAND_OFF = 0x00,
    TS_COMMAND_ON = 0x02
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
    /*!
     * \brief Current physical status of the TS.
     * \details Represents the last confirmed state received from the CAN bus 
     */
    enum TsStatus status;

    /*!
     * \brief Pending "Power On" request flag.
     * \details Set to true when the driver initiates the start sequence. 
     * Automatically cleared once the status transition to TS_STATUS_ON is confirmed.
     */
    bool request_on;

    /*!
     * \brief Pending "Power Off" request flag.
     * \details Set to true when a shutdown is commanded. 
     * Automatically cleared once the status transition to TS_STATUS_OFF is confirmed.
     */
    bool request_off;
};

/*!
 * \brief Initializes the TS handler and internal variables.
 */
void TS_init();

/*!
 * \brief Updates the internal state based on a status byte.
 * \param status_byte Raw status code received over CAN.
 */
void TS_update_from_can(uint8_t status_byte);

/*!
 * \brief Returns the current filtered status of the Tractive System.
 */
enum TsStatus TS_get_status();

/*!
 * \brief Utility function to get the string representation of a state.
 */
const char *TS_get_state_name(enum TsStatus status);

/*!
 * \brief Set a request to transition to TS_ON. 
 */
void TS_request_power_on();

/*!
 * \brief Set a request to transition to TS_OFF.
 */
void TS_request_power_off();

/*!
 * \brief Checks if a Power On request is pending.
 */
bool TS_is_power_on_requested();

/*!
 * \brief Checks if a Power Off request is pending.
 */
bool TS_is_power_off_requested();

/*!
 * \brief Clears (resets) all pending requests.
 */
void TS_clear_request();

#endif