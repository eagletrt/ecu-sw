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
 * \param ptt_control A callback that drives the PTT output pin.
 *
 * \retval VEHICLE_RC_OK on success.
 * \retval VEHICLE_RC_NULL_POINTER if \p ptt_control is NULL.
 */
enum VehicleReturnCode vehicle_api_init(vehicle_ptt_control_callback ptt_control);

/*!
 * \brief Sets the internal edge-triggered Tractive System activation request flag.
 *
 * \details Fed from the steering-wheel button status received over CAN.
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

/*!
 * \brief Drives the PTT output pin.
 *
 * \details Mirrors the steering-wheel PTT button (received over CAN) onto the pin
 *     through the callback supplied at \ref vehicle_api_init.
 *
 * \param state true to assert the PTT line, false to release it.
 *
 * \retval VEHICLE_RC_OK on success.
 * \retval VEHICLE_RC_ERROR if no control callback was configured.
 */
enum VehicleReturnCode vehicle_api_set_ptt(bool state);

#endif
