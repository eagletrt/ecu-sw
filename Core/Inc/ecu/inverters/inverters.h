/*!
 * \file inverters.h
 * \author Dorijan Di Zepp
 * \date 2026-05-08
 * \brief Hardware-agnostic module for inverters control.
 *
 * This module defines the inverters handler, the return codes and the callbacks signatures
 */

#ifndef INVERTERS_H
#define INVERTERS

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define BATTERY_POWER_W_MAX (80000.0f) /* Maximum battery power allowed by Formula Student rules (80kW). */

#define RPM_TO_RADS_COEFF ((2 * M_PI) / 60.0f) /* Conversion factor: RPM to radians per second. */

#define RADS_TO_RPM_COEFF (60.0f / (2 * M_PI)) /* Conversion factor: Radians per second to RPM. */

#define INVERTER_I_MAX (74.0f) /* Maximum continuous phase current (Arms). */

#define INVERTER_I_PEAK (120.0f) /* Absolute peak phase current (Arms) before software trip. */

#define V_DC_MAX (610.0f) /* Maximum safe operating DC-link voltage to protect capacitors. */

/*
TODO:
the following constants require verification
to see if the values associated are valid
*/
#define MOTOR_TORQUE_NM_PEAK (21.0f) /* Maximum mechanical torque allowed per motor (Nm). */

#define MOTOR_TORQUE_NM_ARMS_COEFF (0.25f) /* Torque constant: Ratio of torque produced per Ampere RMS (Nm/Arms). */

#define MOTOR_POWER_W_MAX (60000.0f) /* Maximum mechanical power allowed per motor (W). */

#define HV_MAX_REGEN_CURRENT (-24.0f) /* Maximum allowable regenerative current into the battery (A). */

#define HV_MIN_CELL_VOLTAGE (2.8f) /* Minimum safe voltage for a single battery cell (V). */

#define HV_CELL_COUNT (144) /* Total number of battery cells in series. */

#define BATTERY_POWER_W_MIN (HV_MAX_REGEN_CURRENT * HV_MIN_CELL_VOLTAGE * HV_CELL_COUNT) /* Maximum regenerative power allowed into the battery (W). */

#define BATTERY_I_MAX (140.0f) /* Maximum DC current allowed to be drawn from the battery (A). */

/*!
 * \brief Return codes for the inverters module APIs.
 */
enum InvertersReturnCode {
    INVERTERS_RC_OK,    /*!< Operation completed successfully */
    INVERTERS_RC_ERROR, /*!< Operation NOT completed successfully */
};

/*!
 * \brief Drive command status for the inverter.
 */
enum InvertersDriveStatus {
    INVERTERS_DRIVE_STATUS_ENABLE,  /*!< Request inverter activation */
    INVERTERS_DRIVE_STATUS_DISABLE, /*!< Request inverter deactivation */
};

/*!
 * \brief Axis definition for multi-axle traction control.
 */
enum InvertersAxis {
    INVERTERS_AXIS_FRONT, /*!< Select the inverters of the front axis */
    INVERTERS_AXIS_REAR,  /*!< Select the inverters of the rear axis */
};

/*!
 * \brief Side definition for multi-side traction control.
 */
enum InvertersSide {
    INVERTERS_SIDE_LEFT,  /*!< Select the inverters of the left side */
    INVERTERS_SIDE_RIGHT, /*!< Select the inverters of the right side */
};

/*!
 * \brief Callback to send operational drive commands.
 * \param[in] InvertersDriveStatus The desired operational status.
 * \param[in] InvertersAxis The axis to command.
 * \param[in] InvertersSide The side to command.
 * \retval INVERTERS_RC_OK
 * \retval INVERTERS_RC_ERROR
 */
typedef enum InvertersReturnCode (*inverters_send_drive_command_callback)(enum InvertersDriveStatus, enum InvertersAxis, enum InvertersSide);

/*!
 * \brief Callback to set target torque.
 * \param[in] float Target torque in Newton-meters.
 * \param[in] InvertersAxis The axis to command.
 * \param[in] InvertersSide The side to command.
 * \retval INVERTERS_RC_OK The requested torque has been set.
 * \retval INVERTERS_RC_ERROR It was not possible to change the torque because of an error
 */
typedef enum InvertersReturnCode (*inverters_set_torque_callback)(double, enum InvertersAxis, enum InvertersSide);

/*!
 * \brief Callback to retrieve the current RPM from a specific axis.
 * \param[in] InvertersAxis The axis to query.
 * \param[in] InvertersSide The side to query.
 * \return float Current RPM
 */
typedef float (*inverters_get_rpm_callback)(enum InvertersAxis, enum InvertersSide);

/*!
 * \brief Handler structure for inverter operations.
 */
struct InvertersHandler {
    inverters_send_drive_command_callback send_drive_command; /*!< Pointer to the function that sends Drive/Disable commands to an inverter axis */
    inverters_set_torque_callback set_torque;                 /*!< Pointer to the function that writes torque requests to an inverter axis */
    inverters_get_rpm_callback get_rpm;                       /*!< Pointer to the function that retrieves current RPM telemetry from an inverter axis */
};

#endif