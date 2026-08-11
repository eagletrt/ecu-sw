#ifndef TSAC_API_H
#define TSAC_API_H

#include "tsac.h"

/*!
 * \brief Initializes the internal TSAC handler flags to their default states.
 *
 * \details This function resets the internal flags of the TSAC handler to their default values.
 * It should be called during system initialization to ensure a known starting state.
 *
 * \param get_tick A callback function to retrieve the current system tick count. This function is used for timing and timeout management within the TSAC API.
 *
 * \retval TSAC_RC_OK on success.
 * \retval TSAC_RC_ERROR if an error occurs during initialization.
 */
enum TsacReturnCode tsac_api_init(tsac_get_tick_callback get_tick);

/*!
 * \brief Sets the internal tractive system voltage safety evaluation flag.
 *
 * \param is_higher The boolean state confirming if voltage is above 60V.
 *
 * \retval TSAC_RC_OK on success.
 * \retval TSAC_RC_ERROR if an error occurs while setting the flag.
 */
enum TsacReturnCode tsac_api_set_voltage_higher_than_60v(bool is_higher);

/*!
 * \brief Gets the current status of the tractive system voltage threshold check.
 *
 * \return True if the voltage is verified to be higher than 60V, false otherwise.
 */
bool tsac_api_get_voltage_higher_than_60v(void);

/*!
 * \brief Sets the internal flag indicating whether the tractive system should be required to be enabled.
 *
 * \param state The boolean state to set for the requirement.
 *
 * \retval TSAC_RC_OK on success.
 * \retval TSAC_RC_ERROR if an error occurs while setting the flag.
 */
enum TsacReturnCode tsac_api_set_ts_state_to_require(bool state);

/*!
 * \brief Gets the current state of the internal flag indicating whether the tractive system should be required to be enabled.
 *
 * \return True if the tractive system is required to be enabled, false otherwise.
 */
bool tsac_api_get_ts_state_to_require(void);

/*!
 * \brief Sets the current status of the TSAC mainboard.
 *
 * \param status The new status to set for the TSAC mainboard.
 *
 * \retval TSAC_RC_OK on success.
 * \retval TSAC_RC_ERROR if an error occurs while setting the status.
 */
enum TsacReturnCode tsac_api_set_tsac_status(enum CanPrimaryTsacstatusMainboardstatus status);

/*!
 * \brief Gets the current status of the TSAC mainboard.
 *
 * \return The current status of the TSAC mainboard as reported by the TSAC.
 */
enum CanPrimaryTsacstatusMainboardstatus tsac_api_get_tsac_status(void);

/*!
 * \brief Periodically checks and updates the TSAC status based on the specified tick interval.
 *
 * \retval TSAC_RC_OK on success.
 * \retval TSAC_RC_ERROR if an error occurs during the periodic check.
 */
enum TsacReturnCode tsac_api_periodically_require_tsac_status(void);

/*!
 * \brief Checks if the TSAC status has timed out based on the last received timestamp.
 *
 * \return True if the TSAC status has timed out, false otherwise.
 */
bool tsac_api_is_tsac_status_timeout(void);

#endif // TSAC_API_H
