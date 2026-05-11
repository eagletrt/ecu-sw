/*!
 * \file inverters-api.c
 * \author Dorijan Di Zepp
 * \date 2026-05-11
 * \brief Implementation of inverters module.
 * \details Provides an interface for sending activation/deactivation 
 * commands and requested torque by applying a cut-off safety function.
 */

#include "inverters-api.h"
#include "eagletrt-api.h"

/*!
 * \brief Internal module handler.
 * \details Hidden from external linkage to enforce API-only access.
 */
EAGLETRT_STATIC struct InvertersHandler inverters_handler;

/*!
 * \brief Sanitizes RPM values to prevent division by zero errors.
 * \details If the absolute value of the RPM is below the provided threshold, 
 * the function clamps the value to the threshold while preserving the sign.
 * \param[in] rpm The raw rotational speed value.
 * \param[in] pos_threshold The minimum allowable absolute speed value.
 * \return The sanitized RPM value, never zero.
 */
EAGLETRT_STATIC_INLINE float prv_inverters_avoid_zero_division(float rpm, float pos_threshold) {
    if (rpm > -pos_threshold && rpm < pos_threshold) {
        return ((rpm > 0) ? pos_threshold : -pos_threshold);
    }
    return rpm;
}

/*!
 * \brief Calculates the physical torque limit for an individual motor.
 * \details Protects the motor and inverter hardware by returning the minimum 
 * of the peak mechanical torque and the speed-dependent power limit to prevent 
 * thermal damage.
 * \param[in] rpm The current rotational speed of the motor.
 * \return The maximum allowable absolute torque in Newton-meters (Nm).
 */
EAGLETRT_STATIC_INLINE float prv_inverters_get_motor_torque_limit(const float rpm) {
    // Inverter current limit (Torque = Current * Kt)
    // Limits the phase current to protect inverter's hardware
    float t_max_current = INVERTER_PEAK_CURRENT_A * MOTOR_TORQUE_PER_CURRENT_NM_A;

    // Motor power limit (Torque = Power / Omega)
    float abs_rpm = fabs(rpm);
    float t_max_power = (MOTOR_MAX_MECHANICAL_POWER_W * RAD_TO_RPM_COEFF) / abs_rpm;

    // Return the lowest of the three.
    // This protects the motor and the inverter
    return EAGLETRT_API_MIN(MOTOR_PEAK_TORQUE_NM, EAGLETRT_API_MIN(t_max_current, t_max_power));
}

/*!
 * \brief Calculates the open circuit voltage (VOC) of a single battery cell.
 * \details Uses a 4th-order polynomial regression model to estimate the resting 
 * voltage of a cell based on its state of charge (SOC).
 * \note This model is highly specific to the cell chemistry
 * \return float The estimated resting voltage of a single cell (volts).
 */
float prv_inverters_pack_voc_model(void) {
    float soc = inverters_handler.get_soc();
    soc = fmax(0.0, fmin(1.0, soc));
    //TODO: verify the values as they correspond to the characteristics of the old pack
    // which should be "recycled" for kraken.
    return -3.85189120 * pow(soc, 4) + 9.42278296 * pow(soc, 3) - 8.31949326 * pow(soc, 2) + 4.04805239 * soc + 2.82544823;
}

/*!
 * \brief Calculates the internal resistance of a single battery cell.
 * \details Estimates the electrical "friction" inside the cell based on SOC. 
 * This value is used for predicting the voltage sag (V_drop = I * R).
 * \return float The internal resistance of a single cell (Ohms).
 */
float prv_inverters_internal_resistance_model(void) {
    float soc = inverters_handler.get_soc();
    soc = fmax(0.0, fmin(1.0, soc));
    //TODO: verify the values as they correspond to the characteristics of the old pack
    // which should be "recycled" for kraken.
    return 0.0141 + 0.0021 * soc;
}

/*!
 * \brief Clamps total vehicle torque to stay within a global power limit.
 * \details This function calculates the total instantaneous mechanical power 
 * across all four wheels. If the sum exceeds  the 'power_max', 
 * it calculates a reduction ratio.
 * \param[in] power_max The maximum allowable total power (Watts).
 * \param[in] w_front_left Angular velocity (rad/s) of the front left wheel.
 * \param[in] w_front_right Angular velocity (rad/s) of the front right wheel.
 * \param[in] w_rear_left Angular velocity (rad/s) of the rear left wheel.
 * \param[in] w_rear_right Angular velocity (rad/s) of the rear right wheel.
 * \param[in,out] torque_front_left Pointer to the front left torque; to be scaled on low voltage.
 * \param[in,out] torque_front_right Pointer to the front right torque; to be scaled on low voltage.
 * \param[in,out] torque_rear_left Pointer to the rear left torque; to be scaled on low voltage.
 * \param[in,out] torque_rear_right Pointer to the rear right torque; to be scaled on low voltage.
 */
void prv_inverters_limit_torque_by_power(float power_max, float w_front_left, float w_front_right, float w_rear_left, float w_rear_right, float *torque_front_left, float *torque_front_right, float *torque_rear_left, float *torque_rear_right) {
    // Total mechanical power: P = Sum(T * w)
    float total_p = (*torque_front_left * w_front_left) + (*torque_front_right * w_front_right) + (*torque_rear_left * w_rear_left) + (*torque_rear_right * w_rear_right);
    float reduction_ratio = 0.0F;

    if (fabs(power_max) < 0.5F) {
        // case in which the battery is almost "dead"
        *torque_front_left = 0.0F;
        *torque_front_right = 0.0F;
        *torque_rear_left = 0.0F;
        *torque_rear_right = 0.0F;
    } else if (total_p > power_max && total_p >= 0.0F) {
        // discharge case
        reduction_ratio = EAGLETRT_API_MIN(EAGLETRT_API_MAX(power_max / total_p, 0.0F), 1.0F);
        // Apply the same ratio to all motors
        *torque_front_left *= reduction_ratio;
        *torque_front_right *= reduction_ratio;
        *torque_rear_left *= reduction_ratio;
        *torque_rear_right *= reduction_ratio;
    } else if (total_p < BATTERY_MAX_REGEN_POWER_W && total_p < 0.0F) {
        // regen case
        reduction_ratio = EAGLETRT_API_MIN(EAGLETRT_API_MAX(BATTERY_MAX_REGEN_POWER_W / total_p, 0.0F), 1.0F);
        // Apply the same ratio to all motors
        *torque_front_left *= reduction_ratio;
        *torque_front_right *= reduction_ratio;
        *torque_rear_left *= reduction_ratio;
        *torque_rear_right *= reduction_ratio;
    }
}

/*!
 * \brief High-level wrapper to enforce the regulatory 80kW power limit.
 * \details Converts motor speeds from RPM to angular velocity (rad/s) and invokes 
 * the power limiting logic using the \ref BATTERY_POWER_W_MAX threshold.
 * \param[in] rpm_front_left Current speed of the front left motor (RPM).
 * \param[in] rpm_front_right Current speed of the front right motor (RPM).
 * \param[in] rpm_rear_left Current speed of the rear left motor (RPM).
 * \param[in] rpm_rear_right Current speed of the rear right motor (RPM).
 * \param[in,out] torque_front_left Pointer to the front left torque request.
 * \param[in,out] torque_front_right Pointer to the front right torque request.
 * \param[in,out] torque_rear_left Pointer to the rear left torque request.
 * \param[in,out] torque_rear_right Pointer to the rear right torque request.
 */
EAGLETRT_STATIC_INLINE void prv_inverters_maximum_allowable_power(
    float rpm_front_left, float rpm_front_right, float rpm_rear_left, float rpm_rear_right, float *torque_front_left, float *torque_front_right, float *torque_rear_left, float *torque_rear_right) {

    // Convert speeds to angular velocity (rad/s)
    float w_front_left = rpm_front_left * RPM_TO_RAD_COEFF;
    float w_front_right = rpm_front_right * RPM_TO_RAD_COEFF;
    float w_rear_left = rpm_rear_left * RPM_TO_RAD_COEFF;
    float w_rear_right = rpm_rear_right * RPM_TO_RAD_COEFF;

    prv_inverters_limit_torque_by_power(BATTERY_MAX_POWER_W, w_front_left, w_front_right, w_rear_left, w_rear_right, torque_front_left, torque_front_right, torque_rear_left, torque_rear_right);
}

/*!
 * \brief Protects the battery from under-voltage by scaling torque during voltage sag.
 * \details If the voltage approaches the absolute minimum threshold ( \ref HV_MIN_CELL_VOLTAGE * \ref HV_CELL_COUNT), 
 * it should proportionally reduce torque across all motors to prevent errors.
 * Current implementation is a NOP placeholder.
 * \param[in] rpm_front_left Current speed of the front left motor (RPM).
 * \param[in] rpm_front_right Current speed of the front right motor (RPM).
 * \param[in] rpm_rear_left Current speed of the rear left motor (RPM).
 * \param[in] rpm_rear_right Current speed of the rear right motor (RPM).
 * \param[in,out] torque_front_left Pointer to the front left torque; to be scaled on low voltage.
 * \param[in,out] torque_front_right Pointer to the front right torque; to be scaled on low voltage.
 * \param[in,out] torque_rear_left Pointer to the rear left torque; to be scaled on low voltage.
 * \param[in,out] torque_rear_right Pointer to the rear right torque; to be scaled on low voltage.
 */
EAGLETRT_STATIC_INLINE void prv_inverters_minimum_cell_voltage_limit(float rpm_front_left, float rpm_front_right, float rpm_rear_left, float rpm_rear_right, float *torque_front_left, float *torque_front_right, float *torque_rear_left, float *torque_rear_right) {
    float w_front_left = rpm_front_left * RPM_TO_RAD_COEFF;
    float w_front_right = rpm_front_right * RPM_TO_RAD_COEFF;
    float w_rear_left = rpm_rear_left * RPM_TO_RAD_COEFF;
    float w_rear_right = rpm_rear_right * RPM_TO_RAD_COEFF;

    float VOC = prv_inverters_pack_voc_model();
    float dV = EAGLETRT_API_MAX(VOC - HV_MIN_CELL_VOLTAGE_V, 0.0);
    float r = prv_inverters_internal_resistance_model();
    float i_max = dV / r;
    i_max *= BATTERY_PARALLELS;

    float packV = VOC * HV_CELL_COUNT;
    float p_max = packV * i_max;
    prv_inverters_limit_torque_by_power(p_max, w_front_left, w_front_right, w_rear_left, w_rear_right, torque_front_left, torque_front_right, torque_rear_left, torque_rear_right);
}

/*!
 * \brief Orchestrates the safety cut-off pipeline for all four inverters.
 * \details This function acts as the "overseer" of the powertrain. It processes raw torque 
 * requests (which can be identical for all wheels or different if using for e.g. torque vectoring) 
 * and ensures they stay within the "Safe Operating Envelope" defined by the rules 
 * and hardware limits.
 *
 * 1. INDIVIDUAL HARDWARE CLAMPING
 * Each wheel is checked against its physical limits (21Nm peak, 90A inverter current, 
 * and 20kW power). If a motor is spinning too fast to produce the requested 
 * torque safely, it is clipped.
 * 
 * 2. TOTAL POWER ASSESSMENT
 * The function sums the (now clipped) torques and multiplies by the current RPM 
 * to find the total power (kW) the car is attempting to use or regenerate.
 *
 * 3. GLOBAL SCALING
 * If the total power exceeds the 80kW limit or the battery limit, 
 * the function calculates a reduction ratio.
 * 
 * 4. PROPORTIONALITY PRESERVATION
 * Instead of cutting power to just one motor, the reduction ratio is applied 
 * to all four wheels equally. This ensures that if the car was requested to 
 * deliver 50/50 or 60/40 torque distribution, that balance is maintained 
 * even during a power cut, preventing unexpected vehicle yaw (spinning).
 * 
 * \param[in,out] torque_front_left_nm  Pointer to front left request. Modified if limits are hit.
 * \param[in,out] torque_front_right_nm Pointer to front right request. Modified if limits are hit.
 * \param[in,out] torque_rear_left_nm   Pointer to rear left request. Modified if limits are hit.
 * \param[in,out] torque_rear_right_nm  Pointer to rear right request. Modified if limits are hit.
 */
EAGLETRT_STATIC_INLINE void prv_inverters_apply_cut_off(float *torque_front_left_nm, float *torque_front_right_nm, float *torque_rear_left_nm, float *torque_rear_right_nm) {

    const float speed_threshold = 0.1F;
    float rpm_front_left = prv_inverters_avoid_zero_division(inverters_handler.get_rpm(INVERTERS_POSITION_FRONT_LEFT), speed_threshold);
    float rpm_front_right = prv_inverters_avoid_zero_division(inverters_handler.get_rpm(INVERTERS_POSITION_FRONT_RIGHT), speed_threshold);
    float rpm_rear_left = prv_inverters_avoid_zero_division(inverters_handler.get_rpm(INVERTERS_POSITION_REAR_LEFT), speed_threshold);
    float rpm_rear_right = prv_inverters_avoid_zero_division(inverters_handler.get_rpm(INVERTERS_POSITION_REAR_RIGHT), speed_threshold);

    // Individual Hardware Protection
    // Clamping each motor to its physical peak (does not affect relative ratios)
    float limit_front_left = prv_inverters_get_motor_torque_limit(rpm_front_left);
    *torque_front_left_nm = EAGLETRT_API_CLAMP(*torque_front_left_nm, -limit_front_left, limit_front_left);

    float limit_front_right = prv_inverters_get_motor_torque_limit(rpm_front_right);
    *torque_front_right_nm = EAGLETRT_API_CLAMP(*torque_front_right_nm, -limit_front_right, limit_front_right);

    float limit_rear_left = prv_inverters_get_motor_torque_limit(rpm_rear_left);
    *torque_rear_left_nm = EAGLETRT_API_CLAMP(*torque_rear_left_nm, -limit_rear_left, limit_rear_left);

    float limit_rear_right = prv_inverters_get_motor_torque_limit(rpm_rear_right);
    *torque_rear_right_nm = EAGLETRT_API_CLAMP(*torque_rear_right_nm, -limit_rear_right, limit_rear_right);

    // Voltage sag protection
    // If the given throttle request will drop the voltage too much (risk of sag)
    // automatically cuts the torque
    // LIMIT TYPE: Bottom-up (don't drop below Y)
    prv_inverters_minimum_cell_voltage_limit(rpm_front_left, rpm_front_right, rpm_rear_left, rpm_rear_right, torque_front_left_nm, torque_front_right_nm, torque_rear_left_nm, torque_rear_right_nm);

    // Global battery protection and regulation check
    // This scales all motors proportionally to stay under 80kW and regen limits
    // LIMIT TYPE: Top-down (don't exceed X)
    prv_inverters_maximum_allowable_power(rpm_front_left, rpm_front_right, rpm_rear_left, rpm_rear_right, torque_front_left_nm, torque_front_right_nm, torque_rear_left_nm, torque_rear_right_nm);
}

enum InvertersReturnCode inverters_api_init(
    inverters_send_drive_command_callback send_drive_command,
    inverters_set_torque_callback set_torque,
    inverters_get_rpm_callback get_rpm,
    inverters_get_soc_callback get_soc) {
    // Validate callback pointers
    if ((send_drive_command == NULL) || (set_torque == NULL) || (get_rpm == NULL) || (get_soc == NULL)) {
        return INVERTERS_RC_ERROR;
    }

    // Store callbacks
    inverters_handler.send_drive_command = send_drive_command;
    inverters_handler.set_torque = set_torque;
    inverters_handler.get_rpm = get_rpm;
    inverters_handler.get_soc = get_soc;

    // By default, disable all the inverters
    return inverters_api_set_drive(INVERTERS_DRIVE_STATUS_DISABLE);
}

enum InvertersReturnCode inverters_api_set_drive(enum InvertersDriveStatus drive_status) {
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

enum InvertersReturnCode inverters_api_set_torque(float torque_front_left_nm, float torque_front_right_nm, float torque_rear_left_nm, float torque_rear_right_nm) {
    if (inverters_handler.set_torque == NULL || inverters_handler.get_rpm == NULL || inverters_handler.get_soc == NULL) {
        return INVERTERS_RC_ERROR;
    }

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