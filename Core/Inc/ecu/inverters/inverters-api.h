/*!
 * \file inverters-api.h
 * \author Dorijan Di Zepp
 * \date 2026-05-19
 * \brief Public API for the inverters control module.
 */

#ifndef INVERTERS_API_H
#define INVERTERS_API_H

#include "inverters.h"

/*!
 * \brief Initializes the inverters handler and disable all inverters for safety.
 * \warning Initialization fails if hardware configuration is invalid or
 * inverters cannot be placed in a safe state.
 * \param[in] send_drive_command Pointer to callback for sending enable/disable commands.
 * \param[in] set_torque Pointer to callback for torque requests.
 * \retval INVERTERS_RC_OK If module is fully initialized and ready.
 * \retval INVERTERS_RC_ERROR If initialization, verification or safety failed.
 */
enum InvertersReturnCode inverters_api_init(
    inverters_send_drive_command_callback send_drive_command,
    inverters_set_torque_callback set_torque);

/*!
 * \brief Sets the operational drive status for all inverters.
 * \param[in] drive_status The requested status.
 * \retval INVERTERS_RC_OK If the command was sent successfully.
 * \retval INVERTERS_RC_ERROR If the command failed even for a single inverter or the \ref drive_status is not valid.
 */
enum InvertersReturnCode inverters_api_set_drive_status(enum InvertersDriveStatus drive_status);

/*!
 * \brief Dispatches target torque requests to all four inverters.
 * \details Applies internal "cut-off" safety layer in order to not exceed the maximum battery 
 * power allowed and not damaging the internal battery pack. 
 * This layer ensures the 80kW power limit and battery current constraints 
 * are respected by scaling the four requests proportionally before CAN transmission.
 * \param[in] torques Array containing the requested torque (Nm) for each wheel. 
 * The array must be sized to \ref INVERTERS_POSITION_COUNT.
 * \retval INVERTERS_RC_OK Command validated, limited and transmitted.
 * \retval INVERTERS_RC_ERROR Error during communication.
 */
enum InvertersReturnCode inverters_api_set_torque(const float torques[INVERTERS_POSITION_COUNT]);

/*!
 * \brief Set the current rpm of all 4 motors.
 * \param[in] rpm_motors Array containing the current rpm of all 4 motors.
 * The array must be sized to \ref INVERTERS_POSITION_COUNT.
 * \retval INVERTERS_RC_OK The values have been copied into the handler.
 * \retval INVERTERS_RC_ERROR A problem occurred while copying the values into the handler of the array is \c NULL.
 */
enum InvertersReturnCode inverters_api_set_rpm_motors(float rpm_motors[INVERTERS_POSITION_COUNT]);

/*!
 * \brief Set the current SoC (State of Charge) of the battery.
 * \details The value is automatically clamped into the range 0.0 and 1.0
 * which represents the percentage of the battery.
 * \param[in] hv_bms_soc The current State of Charge of the battery.
 * \retval INVERTERS_RC_OK The values has been copied correctly.
 */
enum InvertersReturnCode inverters_api_set_hv_bms_soc(float hv_bms_soc);

#endif