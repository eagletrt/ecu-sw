/*!
 * \file inverters-api.c
 * \author Dorijan Di Zepp
 * \date 2026-05-10
 * \brief Implementation of inverters module.
 * \details Provides an interface for sending activation/deactivation 
 * commands and requested torque.
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
 * \brief High-level wrapper to enforce the regulatory 80kW power limit.
 * \details Converts motor speeds from RPM to angular velocity (rad/s) and invokes 
 * the power limiting logic using the \ref BATTERY_POWER_W_MAX threshold.
 * \param[in] rpm_front_left Current speed of the front-left motor (RPM).
 * \param[in] rpm_front_right Current speed of the front-right motor (RPM).
 * \param[in] rpm_rear_left Current speed of the rear-left motor (RPM).
 * \param[in] rpm_rear_right Current speed of the rear-right motor (RPM).
 * \param[in,out] torque_front_left Pointer to the FL torque request.
 * \param[in,out] torque_front_right Pointer to the FR torque request.
 * \param[in,out] torque_rear_left Pointer to the RL torque request.
 * \param[in,out] torque_rear_right Pointer to the RR torque request.
 */
EAGLETRT_STATIC_INLINE void prv_inverters_maximum_allowable_power(
    float rpm_front_left, float rpm_front_right, float rpm_rear_left, float rpm_rear_right, float *torque_front_left, float *torque_front_right, float *torque_rear_left, float *torque_rear_right) {

    // Convert speeds to angular velocity (rad/s)
    float w_front_left = rpm_front_left * RPM_TO_RAD_COEFF;
    float w_front_right = rpm_front_right * RPM_TO_RAD_COEFF;
    float w_rear_left = rpm_rear_left * RPM_TO_RAD_COEFF;
    float w_rear_right = rpm_rear_right * RPM_TO_RAD_COEFF;

    // Total mechanical power: P = Sum(T * w)
    float total_p = (*torque_front_left * w_front_left) + (*torque_front_right * w_front_right) + (*torque_rear_left * w_rear_left) + (*torque_rear_right * w_rear_right);

    // Total DC current estimation: I = P / V_batt
    // TODO: this is an hard-coded value. Would it be better
    // to fetch, from a callback, the current battery voltage?
    // e.g. float v_batt = get_battery_voltage()
    float v_batt = HV_MIN_CELL_VOLTAGE_V * HV_CELL_COUNT;
    float total_i_dc = fabsf(total_p / v_batt);

    float reduction_ratio = 1.0F;

    // Check 80kW limit
    if (total_p > BATTERY_MAX_POWER_W) {
        reduction_ratio = EAGLETRT_API_MIN(reduction_ratio, BATTERY_MAX_POWER_W / total_p);
    }

    // Check battery DC current limit
    if (total_i_dc > BATTERY_MAX_CURRENT_A) {
        float p_at_i_limit = BATTERY_MAX_CURRENT_A * v_batt;
        reduction_ratio = EAGLETRT_API_MIN(reduction_ratio, p_at_i_limit / total_p);
    }

    // Check regen Limit
    if (total_p < BATTERY_MAX_REGEN_POWER_W) {
        reduction_ratio = EAGLETRT_API_MIN(reduction_ratio, BATTERY_MAX_REGEN_POWER_W / total_p);
    }

    // Apply the same ratio to all motors
    *torque_front_left *= reduction_ratio;
    *torque_front_right *= reduction_ratio;
    *torque_rear_left *= reduction_ratio;
    *torque_rear_right *= reduction_ratio;
}

/*!
 * \brief Protects the battery from under-voltage by scaling torque during voltage sag.
 * \details If the voltage approaches the absolute minimum threshold ( \ref HV_MIN_CELL_VOLTAGE * \ref HV_CELL_COUNT), 
 * it should proportionally reduce torque across all motors to prevent errors.
 * Current implementation is a NOP placeholder.
 * \param[in] rpm_front_left Current speed of the front-left motor (RPM).
 * \param[in] rpm_front_right Current speed of the front-right motor (RPM).
 * \param[in] rpm_rear_left Current speed of the rear-left motor (RPM).
 * \param[in] rpm_rear_right Current speed of the rear-right motor (RPM).
 * \param[in,out] torque_front_left Pointer to the FL torque; to be scaled on low voltage.
 * \param[in,out] torque_front_right Pointer to the FR torque; to be scaled on low voltage.
 * \param[in,out] torque_rear_left Pointer to the RL torque; to be scaled on low voltage.
 * \param[in,out] torque_rear_right Pointer to the RR torque; to be scaled on low voltage.
 */
EAGLETRT_STATIC_INLINE void prv_inverters_minimum_cell_voltage_limit(float rpm_front_left, float rpm_front_right, float rpm_rear_left, float rpm_rear_right, float *torque_front_left, float *torque_front_right, float *torque_rear_left, float *torque_rear_right) {
    /*
    TODO:
    required an implementation of this function to reduce requested torque
    if battery voltage is too low
    */
    EAGLETRT_API_UNUSED(rpm_front_left);
    EAGLETRT_API_UNUSED(rpm_front_right);
    EAGLETRT_API_UNUSED(rpm_rear_left);
    EAGLETRT_API_UNUSED(rpm_rear_right);
    EAGLETRT_API_UNUSED(torque_front_left);
    EAGLETRT_API_UNUSED(torque_front_right);
    EAGLETRT_API_UNUSED(torque_rear_left);
    EAGLETRT_API_UNUSED(torque_rear_right);
    EAGLETRT_API_NOP();
}

/*!
 * \brief Orchestrates the safety cut-off pipeline for all four inverters.
 * \details This is the core safety function of the inverters module. It performs 
 * the following steps in sequence:
 * 1. Calculates the torque distribution ratio among all wheels.
 * 2. Fetches and sanitizes RPM data for each motor.
 * 3. Computes individual current limits (positive and negative) based on the motor 
 * ratio and hardware constraints.
 * 4. Clamps each wheel's torque request to its calculated safe threshold.
 * 5. Applies global battery protections (voltage sag and 80kW power limit).
 * \param[in,out] torque_front_left_nm Pointer to front-left torque request.
 * \param[in,out] torque_front_right_nm Pointer to front-right torque request.
 * \param[in,out] torque_rear_left_nm Pointer to rear-left torque request.
 * \param[in,out] torque_rear_right_nm Pointer to rear-right torque request.
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

    // Global battery protection and regulation check
    // This scales all motors proportionally to stay under 80kW and regen limits
    prv_inverters_maximum_allowable_power(rpm_front_left, rpm_front_right, rpm_rear_left, rpm_rear_right, torque_front_left_nm, torque_front_right_nm, torque_rear_left_nm, torque_rear_right_nm);

    // Voltage Sag Protection
    prv_inverters_minimum_cell_voltage_limit(rpm_front_left, rpm_front_right, rpm_rear_left, rpm_rear_right, torque_front_left_nm, torque_front_right_nm, torque_rear_left_nm, torque_rear_right_nm);
}

enum InvertersReturnCode inverters_api_init(
    inverters_send_drive_command_callback send_drive_command,
    inverters_set_torque_callback set_torque,
    inverters_get_rpm_callback get_rpm) {
    // Validate callback pointers
    if ((send_drive_command == NULL) || (set_torque == NULL) || (get_rpm == NULL)) {
        return INVERTERS_RC_ERROR;
    }

    // Store callbacks
    inverters_handler.send_drive_command = send_drive_command;
    inverters_handler.set_torque = set_torque;
    inverters_handler.get_rpm = get_rpm;

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
    if (inverters_handler.set_torque == NULL || inverters_handler.get_rpm == NULL) {
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