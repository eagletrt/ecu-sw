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
 *
 * \retval VEHICLE_RC_OK on success.
 */
enum VehicleReturnCode vehicle_api_init(void);

/*!
 * \brief Sets the internal edge-triggered Tractive System activation request flag.
 *
 * \param pressed The boolean state to apply to the trigger flag.
 *
 * \retval VEHICLE_RC_OK on success.
 */
enum VehicleReturnCode vehicle_api_set_ts_on_button_pressed(bool pressed);

/*!
 * \brief Gets the current state of the internal Tractive System activation request flag.
 *
 * \return The current boolean state of the flag.
 */
bool vehicle_api_get_ts_on_button_pressed(void);

#endif
