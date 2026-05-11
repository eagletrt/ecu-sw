/*!
 * \file inverters-api.h
 * \author Dorijan Di Zepp
 * \date 2026-05-11
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
 * \param[in] get_rpm Pointer to callback for inverters' rpm.
 * \param[in] get_soc Pointer to callback for battery's soc.
 * \retval INVERTERS_RC_OK If module is fully initialized and ready.
 * \retval INVERTERS_RC_ERROR If initialization, verification or safety failed.
 */
enum InvertersReturnCode inverters_api_init(
    inverters_send_drive_command_callback send_drive_command,
    inverters_set_torque_callback set_torque,
    inverters_get_rpm_callback get_rpm,
    inverters_get_soc_callback get_soc);

/*!
 * \brief Sets the operational drive status for all inverters.
 * \param[in] drive_status The requested status.
 * \retval INVERTERS_RC_OK If the command was sent successfully.
 * \retval INVERTERS_RC_ERROR If the command failed even for a single inverter or the \ref drive_status is not valid.
 */
enum InvertersReturnCode inverters_api_set_drive(enum InvertersDriveStatus drive_status);

/*!
 * \brief Dispatches target torque requests to all four inverters.
 * \details Applies internal "cut-off" safety layer in order to not exceed the maximum battery 
 * power allowed and not damaging the internal battery pack. 
 * This layer ensures the 80kW power limit and battery current constraints 
 * are respected by scaling the four requests proportionally before CAN transmission.
 * \param[in] torque_fl_nm Requested torque (Nm) for the front-left inverter.
 * \param[in] torque_fr_nm Requested torque (Nm) for the front-right inverter.
 * \param[in] torque_rl_nm Requested torque (Nm) for the rear-left inverter.
 * \param[in] torque_rr_nm Requested torque (Nm) for the rear-right inverter.
 * \retval INVERTERS_RC_OK Command validated, limited and transmitted.
 * \retval INVERTERS_RC_ERROR Error during communication.
 */
enum InvertersReturnCode inverters_api_set_torque(float torque_front_left_nm, float torque_front_right_nm, float torque_rear_left_nm, float torque_rear_right_nm);

#endif