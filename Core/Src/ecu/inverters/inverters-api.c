/*!
 * \file inverters-api.c
 * \author Dorijan Di Zepp
 * \date 2026-05-21
 * \brief Implementation of inverters module.
 * \details Provides an interface for sending activation/deactivation 
 * commands and requested torque by applying a cut-off safety function.
 */

#include "inverters-api.h"
#include "eagletrt-api.h"
#include <string.h>

/*!
 * \brief Internal module handler.
 * \details Hidden from external linkage to enforce API-only access.
 */
EAGLETRT_STATIC struct InvertersHandler inverters_handler;

/*!
 * \brief Calculates a global hardware reduction ratio to keep all motors within limits.
 * \details This function implements a "ratio preservation" strategy. It compares each 
 * requested torque against the motor's specific physical limit. 
 * If any motor exceeds its limit, the function calculates a scaling factor (0.0 to 1.0).
 * By returning the smallest (most restrictive) ratio found across all four wheels, 
 * the caller can scale the entire vehicle's torque request uniformly. This ensures 
 * that the vehicle respects hardware constraints while maintaining the intended 
 * torque-vectoring balance, preventing unpredictable handling.
 * \param[in] torque_front_left   The raw torque requested for the front-left motor (Nm).
 * \param[in] torque_front_right  The raw torque requested for the front-right motor (Nm).
 * \param[in] torque_rear_left    The raw torque requested for the rear-left motor (Nm).
 * \param[in] torque_rear_right   The raw torque requested for the rear-right motor (Nm).
 * \param[in] limit_front_left    The maximum allowable torque for the front-left motor at current RPM (Nm).
 * \param[in] limit_front_right   The maximum allowable torque for the front-right motor at current RPM (Nm).
 * \param[in] limit_rear_left     The maximum allowable torque for the rear-left motor at current RPM (Nm).
 * \param[in] limit_rear_right    The maximum allowable torque for the rear-right motor at current RPM (Nm).
 * \return float A scaling factor from 0.0 to 1.0. Returns 1.0 if no motors exceed their limits.
 */
EAGLETRT_STATIC float prv_inverters_get_motors_reduction(
    float torque_front_left, float torque_front_right, float torque_rear_left, float torque_rear_right, float limit_front_left, float limit_front_right, float limit_rear_left, float limit_rear_right) {
    float global_reduction = 1.0F;

    // Create arrays to iterate through the 4 wheels
    float requests[INVERTERS_POSITION_COUNT] = { torque_front_left, torque_front_right, torque_rear_left, torque_rear_right };
    float limits[INVERTERS_POSITION_COUNT] = { limit_front_left, limit_front_right, limit_rear_left, limit_rear_right };

    for (int i = 0; i < INVERTERS_POSITION_COUNT; i++) {
        float absolute_request = fabsf(requests[i]);

        // If the request exceeds the limit, calculate the necessary reduction
        if (absolute_request > limits[i] && absolute_request > 0.001F) {
            float local_ratio = limits[i] / absolute_request;
            // Keep the smallest ratio (the most restrictive cut)
            global_reduction = EAGLETRT_API_MIN(global_reduction, local_ratio);
        }
    }

    return global_reduction;
}

/*!
 * \brief Calculates the physical torque limit for an individual motor based on current RPM.
 * \details Protects the powertrain by returning the most restrictive of three limits:
 * 1. Motor peak mechanical torque (physical saturation).
 * 2. Inverter continuous current limit (thermal/electrical protection).
 * 3. Mechanical power limit (governed by \ref MOTOR_MAX_MECHANICAL_POWER_W).
 * \note Applies a check on rpm value to prevent division by zero errors using the
 * threshold \ref RPM_SPEED_THRESHOLD.
 * \param[in] rpm The current rotational speed of the motor. Sign is ignored via fabsf.
 * \return The maximum allowable absolute torque in Newton-meters (Nm), always positive.
 */
EAGLETRT_STATIC float prv_inverters_get_motor_torque_limit(const float rpm) {
    // Inverter current limit (Torque = Current * Kt)
    // Limits the phase current to protect inverter's hardware
    // using INVERTER_MAX_CONTINUOUS_CURRENT_A instead of INVERTER_PEAK_CURRENT_A
    // will provide less power but can operate for much longer times safely
    float current_max_current = INVERTERS_INVERTER_MAX_CONTINUOUS_CURRENT_A * INVERTERS_MOTOR_TORQUE_PER_CURRENT_NM_A;

    // Ensure rpm is not zero to avoid division by zero
    float absolute_rpm = fabsf(rpm);
    absolute_rpm = EAGLETRT_API_MAX(absolute_rpm, INVERTERS_RPM_SPEED_THRESHOLD);

    // Motor power limit (Torque = Power / Omega)
    const float current_max_torque = INVERTERS_MOTOR_MAX_MECHANICAL_POWER_W / (absolute_rpm * INVERTERS_RPM_TO_RAD_COEFFICIENT);

    // Return the lowest of the three.
    // This protects the motor and the inverter
    return EAGLETRT_API_MIN(INVERTERS_MOTOR_PEAK_TORQUE_NM, EAGLETRT_API_MIN(current_max_current, current_max_torque));
}

/*!
 * \brief Estimates the Open Circuit Voltage (VOC) of a single battery cell.
 * \details Uses a 4th-order polynomial based on the State of Charge (SOC) to 
 * predict the cell voltage when no load is applied. This estimation is critical
 * for calculating the available power headroom before hitting the voltage floor.
 * \note This model is dependant on the cells' characteristics.
 * \return float Estimated voltage per cell (V). Clamped between the model's 0% and 100% values.
 */
float prv_inverters_pack_voc_model(void) {
    float soc = inverters_handler.hv_bms_soc;
    //TODO: verify the values as they correspond to the characteristics of the old pack
    // which should be "recycled" for kraken.
    // It may be possible to retrieve directly the VOC value from the SOC's CAN frame, to be checked.
    return -3.85189120F * powf(soc, 4) + 9.42278296F * powf(soc, 3) - 8.31949326F * powf(soc, 2) + 4.04805239F * soc + 2.82544823F;
}

/*!
 * \brief Estimates the DC internal resistance of a single battery cell.
 * \details Provides the estimated resistance used to predict voltage sag 
 * (V_sag = I_total * R_int). The model follows a linear relationship 
 * where resistance slightly increases with State of Charge.
 * \note This model is dependant on the cells' characteristics.
 * \return float The estimated internal resistance of a single cell in Ohms (Ω).
 */
float prv_inverters_internal_resistance_model(void) {
    float soc = inverters_handler.hv_bms_soc;
    //TODO: verify the values as they correspond to the characteristics of the old pack
    // which should be "recycled" for kraken.
    // It may be possible to retrieve directly the resistance value from the SOC's CAN frame, to be checked.
    return 0.0141F + 0.0021F * soc;
}

/*!
 * \brief Limits total vehicle torque to respect battery power and current constraints.
 * \details Calculates the instantaneous mechanical power and compares it against three limits:
 * 1. The provided 'power_max' (usually the 80kW regulatory limit or voltage sag limit).
 * 2. The physical DC current limit (Battery voltage * Max pack current).
 * 3. The hard-coded battery regeneration limit ( \ref BATTERY_MAX_REGEN_POWER_W ).
 * If any limit is exceeded, a uniform reduction ratio is applied to all wheels to 
 * maintain the torque-vectoring balance while reducing total power consumption or absorption.
 * \param[in] power_max The maximum allowable total power (Watts).
 * \param[in] angular_velocity_front_left Angular velocity (rad/s) of the front left wheel.
 * \param[in] angular_velocity_front_right Angular velocity (rad/s) of the front right wheel.
 * \param[in] angular_velocity_rear_left Angular velocity (rad/s) of the rear left wheel.
 * \param[in] angular_velocity_rear_right Angular velocity (rad/s) of the rear right wheel.
 * \param[in,out] torque_front_left Pointer to the front left torque; to be scaled on low voltage.
 * \param[in,out] torque_front_right Pointer to the front right torque; to be scaled on low voltage.
 * \param[in,out] torque_rear_left Pointer to the rear left torque; to be scaled on low voltage.
 * \param[in,out] torque_rear_right Pointer to the rear right torque; to be scaled on low voltage.
 */
EAGLETRT_STATIC void prv_inverters_limit_torque_by_power(float power_max, float angular_velocity_front_left, float angular_velocity_front_right, float angular_velocity_rear_left, float angular_velocity_rear_right, float *torque_front_left, float *torque_front_right, float *torque_rear_left, float *torque_rear_right) {
    // Total mechanical power: P = Sum(T * w)
    float total_mechanical_power = (*torque_front_left * angular_velocity_front_left) + (*torque_front_right * angular_velocity_front_right) + (*torque_rear_left * angular_velocity_rear_left) + (*torque_rear_right * angular_velocity_rear_right);
    float reduction_ratio = 1.0F;

    // Physical DC current limit
    // Using the Voc model and cell count to find the real-time battery voltage
    float pack_voltage = prv_inverters_pack_voc_model() * INVERTERS_HV_CELL_COUNT;
    float physical_limit = pack_voltage * INVERTERS_HV_MAX_CURRENT_A;

    // Update power_max to the most restrictive limit
    // This ensures we respect both the 80kW rule and the 140A battery limit
    power_max = EAGLETRT_API_MIN(power_max, physical_limit);

    if (fabsf(power_max) < 0.5F) {
        reduction_ratio = 0.0F; // kill torque if battery is almost "dead"
    } else if (total_mechanical_power > power_max && total_mechanical_power >= 0.0F) {
        // discharge and power limit scaling
        reduction_ratio = EAGLETRT_API_MIN(EAGLETRT_API_MAX(power_max / total_mechanical_power, 0.0F), 1.0F);
    } else if (total_mechanical_power < INVERTERS_HV_MAX_REGEN_POWER_W && total_mechanical_power < 0.0F) {
        // regen scaling, avoid  "pushing" more than the cells can absorb
        reduction_ratio = EAGLETRT_API_MIN(EAGLETRT_API_MAX(INVERTERS_HV_MAX_REGEN_POWER_W / total_mechanical_power, 0.0F), 1.0F);
    }

    // Apply the same ratio to all motors
    *torque_front_left *= reduction_ratio;
    *torque_front_right *= reduction_ratio;
    *torque_rear_left *= reduction_ratio;
    *torque_rear_right *= reduction_ratio;
}

/*!
 * \brief High-level wrapper to enforce the regulatory 80kW power limit.
 * \details Scales the requested torques if the total vehicle power exceeds the 
 * maximum allowed by Formula Student rules ( \ref BATTERY_MAX_POWER_W ).
 * It converts motor speeds from RPM to angular velocity (rad/s) and delegates 
 * the scaling math to \ref prv_inverters_limit_torque_by_power.
 * \param[in] rpm_front_left Current speed of the front left motor (RPM).
 * \param[in] rpm_front_right Current speed of the front right motor (RPM).
 * \param[in] rpm_rear_left Current speed of the rear left motor (RPM).
 * \param[in] rpm_rear_right Current speed of the rear right motor (RPM).
 * \param[in,out] torque_front_left Pointer to the front left torque request.
 * \param[in,out] torque_front_right Pointer to the front right torque request.
 * \param[in,out] torque_rear_left Pointer to the rear left torque request.
 * \param[in,out] torque_rear_right Pointer to the rear right torque request.
 */
EAGLETRT_STATIC void prv_inverters_maximum_allowable_power(float rpm_front_left, float rpm_front_right, float rpm_rear_left, float rpm_rear_right, float *torque_front_left, float *torque_front_right, float *torque_rear_left, float *torque_rear_right) {

    // Convert speeds to angular velocity (rad/s)
    float w_front_left = rpm_front_left * INVERTERS_RPM_TO_RAD_COEFFICIENT;
    float w_front_right = rpm_front_right * INVERTERS_RPM_TO_RAD_COEFFICIENT;
    float w_rear_left = rpm_rear_left * INVERTERS_RPM_TO_RAD_COEFFICIENT;
    float w_rear_right = rpm_rear_right * INVERTERS_RPM_TO_RAD_COEFFICIENT;

    prv_inverters_limit_torque_by_power(INVERTERS_HV_MAX_POWER_W, w_front_left, w_front_right, w_rear_left, w_rear_right, torque_front_left, torque_front_right, torque_rear_left, torque_rear_right);
}

/*!
 * \brief Protects the battery from under-voltage by scaling torque during voltage sag.
 * \details If the voltage approaches the absolute minimum threshold ( \ref HV_MIN_CELL_VOLTAGE * \ref HV_CELL_COUNT), 
 * it should proportionally reduce torque across all motors to prevent errors.
 * \param[in] rpm_front_left Current speed of the front left motor (RPM).
 * \param[in] rpm_front_right Current speed of the front right motor (RPM).
 * \param[in] rpm_rear_left Current speed of the rear left motor (RPM).
 * \param[in] rpm_rear_right Current speed of the rear right motor (RPM).
 * \param[in,out] torque_front_left Pointer to the front left torque; to be scaled on low voltage.
 * \param[in,out] torque_front_right Pointer to the front right torque; to be scaled on low voltage.
 * \param[in,out] torque_rear_left Pointer to the rear left torque; to be scaled on low voltage.
 * \param[in,out] torque_rear_right Pointer to the rear right torque; to be scaled on low voltage.
 */
EAGLETRT_STATIC void prv_inverters_minimum_cell_voltage_limit(float rpm_front_left, float rpm_front_right, float rpm_rear_left, float rpm_rear_right, float *torque_front_left, float *torque_front_right, float *torque_rear_left, float *torque_rear_right) {

    float w_front_left = rpm_front_left * INVERTERS_RPM_TO_RAD_COEFFICIENT;
    float w_front_right = rpm_front_right * INVERTERS_RPM_TO_RAD_COEFFICIENT;
    float w_rear_left = rpm_rear_left * INVERTERS_RPM_TO_RAD_COEFFICIENT;
    float w_rear_right = rpm_rear_right * INVERTERS_RPM_TO_RAD_COEFFICIENT;

    float voc = prv_inverters_pack_voc_model();
    float d_v = EAGLETRT_API_MAX(voc - INVERTERS_HV_MIN_CELL_VOLTAGE_V, 0.0);
    float resistance = prv_inverters_internal_resistance_model();
    float i_max = d_v / resistance;
    i_max *= INVERTERS_HV_CELLS_PARALLEL_COUNT;

    float packV = voc * INVERTERS_HV_CELL_COUNT;
    float p_max = packV * i_max;
    prv_inverters_limit_torque_by_power(p_max, w_front_left, w_front_right, w_rear_left, w_rear_right, torque_front_left, torque_front_right, torque_rear_left, torque_rear_right);
}

/*!
 * \brief Orchestrates the safety cut-off pipeline for all four inverters.
 * \details This function acts as the "overseer" of the powertrain. It processes raw torque 
 * requests (which can be identical for all wheels or different if using for e.g. torque vectoring) 
 * and ensures they stay within the "safe operating env" defined by the rules and hardware limits.
 *
 * INDIVIDUAL MOTOR LIMITS
 * Ensures motors/inverters don't exceed mechanical or thermal limits.
 * 
 * VOLTAGE SAG PROTECTION
 * Uses a battery internal resistance model to predict voltage drop. If the requested 
 * current would cause the battery to sag below \ref HV_MIN_CELL_VOLTAGE_V, it calculates 
 * a global power floor and scales all motors proportionally.
 *
 * REGULATION POWER LIMITING
 * Ensures compliance with competition rules and regen limits.
 * By summing the clipped power and applying a single reduction ratio, it preserves 
 * the vehicle's dynamic balance during power cuts.
 * 
 * \param[in,out] torque_front_left_nm  Pointer to front left request. Modified if limits are hit.
 * \param[in,out] torque_front_right_nm Pointer to front right request. Modified if limits are hit.
 * \param[in,out] torque_rear_left_nm   Pointer to rear left request. Modified if limits are hit.
 * \param[in,out] torque_rear_right_nm  Pointer to rear right request. Modified if limits are hit.
 */
EAGLETRT_STATIC void prv_inverters_apply_cut_off(float *torque_front_left_nm, float *torque_front_right_nm, float *torque_rear_left_nm, float *torque_rear_right_nm) {

    // Load current rpm of all motors
    float rpm_front_left = inverters_handler.rpm_motors[INVERTERS_POSITION_FRONT_LEFT];
    float rpm_front_right = inverters_handler.rpm_motors[INVERTERS_POSITION_FRONT_RIGHT];
    float rpm_rear_left = inverters_handler.rpm_motors[INVERTERS_POSITION_REAR_LEFT];
    float rpm_rear_right = inverters_handler.rpm_motors[INVERTERS_POSITION_REAR_RIGHT];

    // Individual hardware protection
    // Calculate the required reduction for each motor independently
    float limit_front_left = prv_inverters_get_motor_torque_limit(rpm_front_left);
    float limit_front_right = prv_inverters_get_motor_torque_limit(rpm_front_right);
    float limit_rear_left = prv_inverters_get_motor_torque_limit(rpm_rear_left);
    float limit_rear_right = prv_inverters_get_motor_torque_limit(rpm_rear_right);

    // Find the "most restricted" motor's ratio
    float motor_reduction = prv_inverters_get_motors_reduction(
        *torque_front_left_nm, *torque_front_right_nm, *torque_rear_left_nm, *torque_rear_right_nm, limit_front_left, limit_front_right, limit_rear_left, limit_rear_right);

    // Apply ratio to all motors
    // This preserves the balance of the car
    *torque_front_left_nm *= motor_reduction;
    *torque_front_right_nm *= motor_reduction;
    *torque_rear_left_nm *= motor_reduction;
    *torque_rear_right_nm *= motor_reduction;

    // Voltage sag protection
    // If the given throttle request will drop the voltage too much (risk of sag),
    // automatically cuts the torque
    prv_inverters_minimum_cell_voltage_limit(rpm_front_left, rpm_front_right, rpm_rear_left, rpm_rear_right, torque_front_left_nm, torque_front_right_nm, torque_rear_left_nm, torque_rear_right_nm);

    // Global battery protection and regulation check
    // This scales all motors proportionally to stay under 80kW and regen limits
    prv_inverters_maximum_allowable_power(rpm_front_left, rpm_front_right, rpm_rear_left, rpm_rear_right, torque_front_left_nm, torque_front_right_nm, torque_rear_left_nm, torque_rear_right_nm);
}

enum InvertersReturnCode inverters_api_init(
    inverters_send_drive_command_callback send_drive_command,
    inverters_set_torque_callback set_torque) {
    // Validate callback pointers
    if ((send_drive_command == NULL) || (set_torque == NULL)) {
        return INVERTERS_RC_ERROR;
    }

    // Store callbacks
    inverters_handler.send_drive_command = send_drive_command;
    inverters_handler.set_torque = set_torque;

    // Make sure rmp_motors values are set to zero
    memset(inverters_handler.rpm_motors, 0, (sizeof(float) * INVERTERS_POSITION_COUNT));

    inverters_handler.hv_bms_soc = 0.0F;

    // By default, disable all the inverters
    return inverters_api_set_drive_status(INVERTERS_DRIVE_STATUS_DISABLE);
}

enum InvertersReturnCode inverters_api_set_drive_status(enum InvertersDriveStatus drive_status) {
    if (inverters_handler.send_drive_command == NULL ||
        (drive_status >= INVERTERS_DRIVE_STATUS_COUNT)) {
        return INVERTERS_RC_ERROR;
    }

    // Send command to all inverters
    // A single failure in the changing of state is not fatal for all the other inverters
    enum InvertersReturnCode return_code = INVERTERS_RC_OK;

    // If any call returns ERROR, rc becomes (and stays) ERROR.
    if (inverters_handler.send_drive_command(drive_status, INVERTERS_POSITION_FRONT_LEFT) != INVERTERS_RC_OK) {
        return_code = INVERTERS_RC_ERROR;
    }

    if (inverters_handler.send_drive_command(drive_status, INVERTERS_POSITION_FRONT_RIGHT) != INVERTERS_RC_OK) {
        return_code = INVERTERS_RC_ERROR;
    }

    if (inverters_handler.send_drive_command(drive_status, INVERTERS_POSITION_REAR_LEFT) != INVERTERS_RC_OK) {
        return_code = INVERTERS_RC_ERROR;
    }

    if (inverters_handler.send_drive_command(drive_status, INVERTERS_POSITION_REAR_RIGHT) != INVERTERS_RC_OK) {
        return_code = INVERTERS_RC_ERROR;
    }

    return return_code;
}

enum InvertersReturnCode inverters_api_set_torque(const float torques[INVERTERS_POSITION_COUNT]) {
    if (inverters_handler.set_torque == NULL || torques == NULL) {
        return INVERTERS_RC_ERROR;
    }

    float torque_front_left_nm = torques[INVERTERS_POSITION_FRONT_LEFT];
    float torque_front_right_nm = torques[INVERTERS_POSITION_FRONT_RIGHT];
    float torque_rear_left_nm = torques[INVERTERS_POSITION_REAR_LEFT];
    float torque_rear_right_nm = torques[INVERTERS_POSITION_REAR_RIGHT];

    // Apply cut off logic to make sure the requested torque doesn't damage the
    // battery pack and/or doesn't exceed the maximum power allowed by rule
    prv_inverters_apply_cut_off(&torque_front_left_nm, &torque_front_right_nm, &torque_rear_left_nm, &torque_rear_right_nm);

    // Attempt to set torque for each inverter regardless of the individual return code
    inverters_handler.set_torque(torque_front_left_nm, INVERTERS_POSITION_FRONT_LEFT);

    inverters_handler.set_torque(torque_front_right_nm, INVERTERS_POSITION_FRONT_RIGHT);

    inverters_handler.set_torque(torque_rear_left_nm, INVERTERS_POSITION_REAR_LEFT);

    inverters_handler.set_torque(torque_rear_right_nm, INVERTERS_POSITION_REAR_RIGHT);

    return INVERTERS_RC_OK;
}

enum InvertersReturnCode inverters_api_set_rpm_motors(float rpm_motors[INVERTERS_POSITION_COUNT]) {
    if (rpm_motors == NULL) {
        return INVERTERS_RC_ERROR;
    }

    // verify that the copy has been successful
    float *returned_pointer = (float *)(memcpy(&(inverters_handler.rpm_motors), rpm_motors, (sizeof(float) * INVERTERS_POSITION_COUNT)));

    return ((float *)returned_pointer == (float *)(&(inverters_handler.rpm_motors))) ? INVERTERS_RC_ERROR : INVERTERS_RC_OK;
}

enum InvertersReturnCode inverters_api_set_hv_bms_soc(float hv_bms_soc) {
    inverters_handler.hv_bms_soc = EAGLETRT_API_CLAMP(hv_bms_soc, 0.0F, 1.0F);

    return INVERTERS_RC_OK;
}