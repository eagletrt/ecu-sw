/*!
 * \file vehicle-api.h
 * \author Dorijan Di Zepp
 * \brief Public API declarations for managing the encapsulated vehicle context.
 */

#ifndef VEHICLE_API_H
#define VEHICLE_API_H

#include "vehicle.h"
#include <stdbool.h>

/*!
 * \brief Initializes the internal vehicle handler flags to their default states.
 * \retval VEHICLE_RC_OK on success.
 */
enum VehicleReturnCode vehicle_api_init(void);

/*!
 * \brief Sets the internal edge-triggered Tractive System activation request flag.
 * \param pressed The boolean state to apply to the trigger flag.
 * \retval VEHICLE_RC_OK on success.
 */
enum VehicleReturnCode vehicle_api_set_ts_on_button_pressed(bool pressed);

/*!
 * \brief Gets the current state of the internal Tractive System activation request flag.
 * \return The current boolean state of the flag.
 */
bool vehicle_api_get_ts_on_button_pressed(void);

/*!
 * \brief Sets the internal tractive system voltage safety evaluation flag.
 *
 * \param is_higher The boolean state confirming if voltage is above 60V.
 *
 * \retval VEHICLE_RC_OK on success.
 */
enum VehicleReturnCode vehicle_api_set_voltage_higher_than_60v(bool is_higher);

/*!
 * \brief Gets the current status of the tractive system voltage threshold check.
 *
 * \return True if the voltage is verified to be higher than 60V.
 */
bool vehicle_api_get_voltage_higher_than_60v(void);

/*!
 * \brief Sets the internal flag indicating whether the tractive system should be required to be enabled.
 *
 * \param state The boolean state to set for the requirement.
 */
void vehicle_api_set_ts_state_to_require(bool state);

/*!
 * \brief Gets the current state of the internal flag indicating whether the tractive system should be required to be enabled.
 *
 * \return True if the tractive system is required to be enabled, false otherwise.
 */
bool vehicle_api_get_ts_state_to_require(void);

/*!
 * \brief Sets the current status of the Tractive System Accumulator Controller (TSAC) as reported by the mainboard.
 *
 * \param status The enumerated status value to set for the TSAC.
 */
void vehicle_api_set_tsac_status(enum CanPrimaryTsacstatusMainboardstatus status);

/*!
 * \brief Gets the current status of the Tractive System Accumulator Controller (TSAC) as reported by the mainboard.
 *
 * \return The enumerated status value of the TSAC.
 */
enum CanPrimaryTsacstatusMainboardstatus vehicle_api_get_tsac_status(void);

/*!
 * \brief Periodically sends the required state of the TSAC (true for TsON, false for Off).
 *
 * \param tick The current system tick count used for timing the periodic check.
 */
void vehicle_api_periodically_require_tsac_status(uint32_t tick);

#endif
