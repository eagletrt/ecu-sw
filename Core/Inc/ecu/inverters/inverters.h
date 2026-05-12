/*!
 * \file inverters.h
 * \author Dorijan Di Zepp
 * \date 2026-05-12
 * \brief Hardware-agnostic module for inverters control.
 *
 * This module defines the inverters handler, the return codes and the callbacks signatures
 * 
 * \note The inverter-specific constants are derived from the "HV-Board_5" model
 * <a href="https://drive.google.com/file/d/1tRlrvqwPYmyLJJesLxHMAl7y99A7nsL0/view?usp=sharing">datasheet</a>.
 * 
 * \note The motor constants defined are derived from the technical specifications of the Fischer TI085 series motors. 
 * These motors are the manufacturer-recommended drive units for the HV-Board_5 inverter 
 * system as specified in the "Recommended Motor" documentation.
 */

#ifndef INVERTERS_H
#define INVERTERS_H

#define _USE_MATH_DEFINES
#include <math.h>

#define BATTERY_MAX_POWER_W (80000.0F) /* Maximum battery power allowed by Formula Student rules (80kW). */

#define RPM_TO_RAD_COEFF ((2 * M_PI) / 60.0F) /* Conversion factor: RPM to radians per second. */

#define INVERTER_MAX_CONTINUOUS_CURRENT_A (74.0F) /* Maximum continuous phase current (Arms). */

#define INVERTER_PEAK_CURRENT_A (90.0F) /* Absolute peak phase current (Arms). */

/*
TODO:
the following constants require verification
to see if the values associated are valid
*/
#define MOTOR_PEAK_TORQUE_NM (21.0F) /* Maximum mechanical torque allowed per motor. */

#define MOTOR_TORQUE_PER_CURRENT_NM_A (0.25F) /* Torque constant (Kt) in Nm/Arms. */

#define MOTOR_MAX_MECHANICAL_POWER_W (20000.0F) /* Maximum mechanical power allowed per motor. */

#define HV_MAX_REGEN_CURRENT_A (-24.0F) /* Maximum allowable regenerative current into the battery. */

#define HV_MIN_CELL_VOLTAGE_V (2.8F) /* Minimum safe voltage for a single battery cell (V). */

#define HV_CELL_COUNT (144) /* Total number of battery cells in series. */

#define BATTERY_MAX_REGEN_POWER_W (HV_MAX_REGEN_CURRENT_A * HV_MIN_CELL_VOLTAGE_V * HV_CELL_COUNT) /* Maximum regenerative power allowed into the battery. */

#define BATTERY_MAX_CURRENT_A (140.0F) /* Maximum DC current allowed to be drawn from the battery. */

#define BATTERY_PARALLELS (3) /* Number of individual battery cells connected in parallel */

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
    INVERTERS_DRIVE_STATUS_COUNT,   /*!< Sentinel value used for input validation */
};

/*!
 * \brief Physical mounting positions of the inverters within the vehicle.
 */
enum InvertersPosition {
    INVERTERS_POSITION_FRONT_LEFT,  /*!< Front axle, left hand side. */
    INVERTERS_POSITION_FRONT_RIGHT, /*!< Front axle, right hand side. */
    INVERTERS_POSITION_REAR_LEFT,   /*!< Rear axle, left hand side. */
    INVERTERS_POSITION_REAR_RIGHT,  /*!< Rear axle, right hand side. */
};

/*!
 * \brief Callback to send operational drive commands.
 * \param[in] InvertersDriveStatus The desired operational status.
 * \param[in] InvertersPosition The inverter to command.
 * \retval INVERTERS_RC_OK The command has been sent.
 * \retval INVERTERS_RC_ERROR A problem occurred where the command couldn't be forwarded.
 */
typedef enum InvertersReturnCode (*inverters_send_drive_command_callback)(enum InvertersDriveStatus, enum InvertersPosition);

/*!
 * \brief Callback to set target torque.
 * \param[in] float Target torque in Newton-meters.
 * \param[in] InvertersPosition The inverter to command.
 * \retval INVERTERS_RC_OK The requested torque has been set.
 * \retval INVERTERS_RC_ERROR It was not possible to change the torque because of an error
 */
typedef enum InvertersReturnCode (*inverters_set_torque_callback)(float, enum InvertersPosition);

/*!
 * \brief Callback to retrieve the current RPM from a specific axis.
 * \param[in] InvertersPosition The inverter to query.
 * \return float Current RPM of the specified inverter
 */
typedef float (*inverters_get_rpm_callback)(enum InvertersPosition);

/**
 * \brief Callback to retrieve the SoC (State of Charge) of the battery pack.
 * \return float Percentage between 0.0 and 1.0 to indicate the battery level
 */
typedef float (*inverters_get_soc_callback)(void);

/*!
 * \brief Handler structure for inverter operations.
 */
struct InvertersHandler {
    inverters_send_drive_command_callback send_drive_command; /*!< Pointer to the function that sends commands to a given inverter */
    inverters_set_torque_callback set_torque;                 /*!< Pointer to the function that writes torque requests to a given inverter */
    inverters_get_rpm_callback get_rpm;                       /*!< Pointer to the function that retrieves current RPM from an inverter */
    inverters_get_soc_callback get_soc;                       /*!< Pointer to the function that retrieves the battery's soc */
};

#endif