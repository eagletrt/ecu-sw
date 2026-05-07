/*!
 * \file inverters-api.c
 * \author Dorijan Di Zepp
 * \date 2026-05-07
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
 * \brief Calculates the peak phase current based on mechanical torque limits.
 * \details Maps the maximum allowable mechanical torque of the motor to the 
 * equivalent RMS phase current using the torque constant (Kt).
 * \return The peak motor current in Amperes RMS (Arms).
 */
EAGLETRT_STATIC_INLINE double inverters_prv_I_mot_peak(void) {
    return MOTOR_TORQUE_NM_PEAK / MOTOR_TORQUE_NM_ARMS_COEFF;
}

/*!
 * \brief Calculates the maximum allowable phase current based on motor power limits.
 * \details Derives the current limit from the maximum mechanical power rating 
 * of the motor, considering the current angular velocity (RPM).
 * \param[in] rpm The current rotational speed of the motor.
 * \return The power-limited motor current in Amperes RMS (Arms).
 */
EAGLETRT_STATIC_INLINE double inverters_prv_I_mot_max(const double rpm) {
    return MOTOR_POWER_W_MAX / MOTOR_TORQUE_NM_ARMS_COEFF * RADS_TO_RPM_COEFF / rpm;
}

/*!
 * \brief Calculates the positive battery current limit for a specific motor.
 * \details Scales the global battery power and current limits based on the 
 * individual motor's torque ratio to ensure total consumption stays within 
 * the 80kW and BATTERY_I_MAX thresholds.
 * \param[in] rpm The current rotational speed of the motor.
 * \param[in] torque_ratio The proportional share of total torque requested for this motor.
 * \return The maximum allowable positive current (Arms) from the battery perspective.
 */
EAGLETRT_STATIC_INLINE double inverters_prv_I_batt_max_positive(const double rpm, const double torque_ratio) {
    return fmin(BATTERY_I_MAX, BATTERY_POWER_W_MAX * torque_ratio / MOTOR_TORQUE_NM_ARMS_COEFF * RADS_TO_RPM_COEFF / rpm);
}

/*!
 * \brief Calculates the negative (regen) battery current limit for a specific motor.
 * \details Scales the regenerative power limits and max regen current based on 
 * the torque ratio to protect the battery pack during braking.
 * \param[in] rpm The current rotational speed of the motor.
 * \param[in] torque_ratio The proportional share of total torque requested for this motor.
 * \return The maximum allowable regenerative current (Arms) as a negative value.
 */
EAGLETRT_STATIC_INLINE double inverters_prv_I_batt_min_negative(const double rpm, const double torque_ratio) {
    return fmax(HV_MAX_REGEN_CURRENT, BATTERY_POWER_W_MIN * torque_ratio / MOTOR_TORQUE_NM_ARMS_COEFF * RADS_TO_RPM_COEFF / rpm);
}

/*!
 * \brief Sanitizes RPM values to prevent division-by-zero errors.
 * \details If the absolute value of the RPM is below the provided threshold, 
 * the function clamps the value to the threshold while preserving the sign.
 * \param[in] rpm The raw rotational speed value.
 * \param[in] pos_threshold The minimum allowable absolute speed value.
 * \return The sanitized RPM value, never zero.
 */
EAGLETRT_STATIC_INLINE double inverters_prv_avoid_zero_division(double rpm, double pos_threshold) {
    if (rpm > -pos_threshold && rpm < pos_threshold) {
        return ((rpm > 0) ? pos_threshold : -pos_threshold);
    }
    return rpm;
}

/*!
 * \brief Computes the absolute maximum allowable positive current (traction).
 * \details Aggregates all upper-bound constraints including hardware peak limits, 
 * motor thermal/mechanical power limits and global battery power limits. 
 * The function returns the most restrictive (lowest) of these values.
 * \param[in] rpm The current rotational speed of the motor.
 * \param[in] torque_ratio The proportional weight of this motor relative to the total request.
 * \return The safe positive current limit in Amperes RMS (Arms).
 */
EAGLETRT_STATIC_INLINE double inverters_prv_compute_current_positive_limit(double rpm, double torque_ratio) {
    double I_mot_peak = inverters_prv_I_mot_peak();
    double I_mot_max = inverters_prv_I_mot_max(rpm);
    double I_batt_max = inverters_prv_I_batt_max_positive(rpm, torque_ratio);
    return fmin(I_mot_peak, fmin(I_mot_max, I_batt_max));
}

/*!
 * \brief Computes the absolute maximum allowable negative current (regeneration).
 * \details Aggregates all lower-bound constraints. Because regenerative current is 
 * expressed as a negative value, the function uses fmax to find the most restrictive 
 * limit (the value closest to zero).
 * \param[in] rpm The current rotational speed of the motor.
 * \param[in] torque_ratio The proportional weight of this motor relative to the total request.
 * \return The safe regenerative current limit in Amperes RMS (Arms) as a negative value.
 */
EAGLETRT_STATIC_INLINE double inverters_prv_compute_current_negative_limit(double rpm, double torque_ratio) {
    double I_mot_peak = -inverters_prv_I_mot_peak();
    double I_mot_max = inverters_prv_I_mot_max(rpm);
    double I_batt_min = inverters_prv_I_batt_min_negative(rpm, torque_ratio);
    return fmax(I_mot_peak, fmax(I_mot_max, I_batt_min));
}

/*!
 * \brief Converts phase current into mechanical torque.
 * \details Applies the motor torque constant (Kt) to convert the calculated 
 * current limit back into a Newton-meter (Nm) value for the inverter command.
 * \param[in] current The phase current in Amperes RMS (Arms).
 * \return The resulting mechanical torque in Newton-meters (Nm).
 */
EAGLETRT_STATIC_INLINE double inverters_prv_current_to_torque(double current) {
    return current * MOTOR_TORQUE_NM_ARMS_COEFF;
}

/*!
 * \brief Limits the combined mechanical power of all four motors to a global threshold.
 * \details Calculates the total instantaneous mechanical power by summing (Torque * Angular Velocity) 
 * for all four wheels. If the sum exceeds \p power_max, a proportional reduction ratio is 
 * calculated and applied to all motors. This preservation of the torque ratio is critical 
 * for maintaining vehicle stability.
 * \param[in] power_max The maximum allowable total mechanical power in Watts (W).
 * \param[in] w_front_left Angular velocity of the front-left motor (rad/s).
 * \param[in] w_front_right Angular velocity of the front-right motor (rad/s).
 * \param[in] w_rear_left Angular velocity of the rear-left motor (rad/s).
 * \param[in] w_rear_right Angular velocity of the rear-right motor (rad/s).
 * \param[in,out] torque_front_left Pointer to the front-left torque; modified if power exceeds limit.
 * \param[in,out] torque_front_right Pointer to the front-right torque; modified if power exceeds limit.
 * \param[in,out] torque_rear_left Pointer to the rear-left torque; modified if power exceeds limit.
 * \param[in,out] torque_rear_right Pointer to the rear-right torque; modified if power exceeds limit.
 */
EAGLETRT_STATIC_INLINE void inverters_prv_limit_torque_by_power(
    double power_max, double w_front_left, double w_front_right, double w_rear_left, double w_rear_right, double *torque_front_left, double *torque_front_right, double *torque_rear_left, double *torque_rear_right) {

    // Calculate total mechanical power: Sum of (Torque * Angular Velocity) for all 4 wheels
    double mechanical_power = (*torque_front_left * w_front_left) +
                              (*torque_front_right * w_front_right) +
                              (*torque_rear_left * w_rear_left) +
                              (*torque_rear_right * w_rear_right);

    double reduction_ratio = 0.0F;

    // Safety check: if max power is set to 0, kill all torque
    if (fabs(power_max) < 0.5) {
        *torque_front_left = 0.0F;
        *torque_front_right = 0.0F;
        *torque_rear_left = 0.0F;
        *torque_rear_right = 0.0F;
    }
    // If we are over the limit and moving forward (positive power)
    else if (mechanical_power > power_max && mechanical_power > 0.0F) {
        reduction_ratio = fmin(fmax(power_max / mechanical_power, 0.0F), 1.0F);

        // Scale all 4 motors by the same ratio to preserve torque distribution
        *torque_front_left *= reduction_ratio;
        *torque_front_right *= reduction_ratio;
        *torque_rear_left *= reduction_ratio;
        *torque_rear_right *= reduction_ratio;
    }
}

/*!
 * \brief High-level wrapper to enforce the regulatory 80kW power limit.
 * \details Converts motor speeds from RPM to angular velocity (rad/s) and invokes 
 * the power limiting logic using the BATTERY_POWER_W_MAX threshold. This is the 
 * primary safeguard against disqualification for exceeding power limits during 
 * dynamic events.
 * \param[in] rpm_front_left Current speed of the Front-Left motor (RPM).
 * \param[in] rpm_front_right Current speed of the Front-Right motor (RPM).
 * \param[in] rpm_rear_left Current speed of the Rear-Left motor (RPM).
 * \param[in] rpm_rear_right Current speed of the Rear-Right motor (RPM).
 * \param[in,out] torque_front_left Pointer to the FL torque request.
 * \param[in,out] torque_front_right Pointer to the FR torque request.
 * \param[in,out] torque_rear_left Pointer to the RL torque request.
 * \param[in,out] torque_rear_right Pointer to the RR torque request.
 */
EAGLETRT_STATIC_INLINE void inverters_prv_maximum_allowable_power(double rpm_front_left, double rpm_front_right, double rpm_rear_left, double rpm_rear_right, double *torque_front_left, double *torque_front_right, double *torque_rear_left, double *torque_rear_right) {
    // Convert all RPMs to angular velocity (rad/s)
    double w_fl = rpm_front_left * RPM_TO_RADS_COEFF;
    double w_fr = rpm_front_right * RPM_TO_RADS_COEFF;
    double w_rl = rpm_rear_left * RPM_TO_RADS_COEFF;
    double w_rr = rpm_rear_right * RPM_TO_RADS_COEFF;

    // Apply the limit using the 80kW threshold
    inverters_prv_limit_torque_by_power(BATTERY_POWER_W_MAX, w_fl, w_fr, w_rl, w_rr, torque_front_left, torque_front_right, torque_rear_left, torque_rear_right);
}

/*!
 * \brief Protects the battery from under-voltage by scaling torque during voltage sag.
 * \details If the voltage approaches the absolute minimum threshold (HV_MIN_CELL_VOLTAGE * HV_CELL_COUNT), 
 * it should proportionally reduce torque across all motors to prevent a BMS-triggered 
 * shutdown. Current implementation is a NOP placeholder.
 * \param[in] rpm_front_left Current speed of the Front-Left motor (RPM).
 * \param[in] rpm_front_right Current speed of the Front-Right motor (RPM).
 * \param[in] rpm_rear_left Current speed of the Rear-Left motor (RPM).
 * \param[in] rpm_rear_right Current speed of the Rear-Right motor (RPM).
 * \param[in,out] torque_front_left Pointer to the FL torque; to be scaled on low voltage.
 * \param[in,out] torque_front_right Pointer to the FR torque; to be scaled on low voltage.
 * \param[in,out] torque_rear_left Pointer to the RL torque; to be scaled on low voltage.
 * \param[in,out] torque_rear_right Pointer to the RR torque; to be scaled on low voltage.
 */
EAGLETRT_STATIC_INLINE void inverters_prv_minimum_cell_voltage_limit(double rpm_front_left, double rpm_front_right, double rpm_rear_left, double rpm_rear_right, double *torque_front_left, double *torque_front_right, double *torque_rear_left, double *torque_rear_right) {
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
 * \details This is the core safety function of the Inverters module. It performs 
 * the following steps in sequence:
 * 1. Calculates the torque distribution ratio among all wheels.
 * 2. Fetches and sanitizes real-time RPM data for each motor.
 * 3. Computes individual current limits (positive and negative) based on the motor 
 * ratio and hardware constraints.
 * 4. Clamps each wheel's torque request to its calculated safe threshold.
 * 5. Applies global battery protections (Voltage Sag and 80kW Power Limit).
 * \param[in,out] torque_front_left_nm Pointer to Front-Left torque request.
 * \param[in,out] torque_front_right_nm Pointer to Front-Right torque request.
 * \param[in,out] torque_rear_left_nm Pointer to Rear-Left torque request.
 * \param[in,out] torque_rear_right_nm Pointer to Rear-Right torque request.
 */
EAGLETRT_STATIC_INLINE void inverters_prv_apply_cut_off(double *torque_front_left_nm, double *torque_front_right_nm, double *torque_rear_left_nm, double *torque_rear_right_nm) {
    // Calculate total absolute torque for the ratio
    double abs_front_left = fabs(*torque_front_left_nm);
    double abs_front_right = fabs(*torque_front_right_nm);
    double abs_rear_left = fabs(*torque_rear_left_nm);
    double abs_rear_right = fabs(*torque_rear_right_nm);

    double total_abs_torque = abs_front_left + abs_front_right + abs_rear_left + abs_rear_right + 0.001F; // epsilon to avoid / 0

    // Calculate individual ratios
    // This ensures that the car cannot request more current than available
    // taking into consideration which inverter requires most
    double ratio_front_left = abs_front_left / total_abs_torque;
    double ratio_front_right = abs_front_right / total_abs_torque;
    double ratio_rear_left = abs_rear_left / total_abs_torque;
    double ratio_rear_right = abs_rear_right / total_abs_torque;

    const double speed_threshold = 0.1F;
    double rpm_front_left = inverters_prv_avoid_zero_division(inverters_handler.get_rpm(INVERTERS_AXIS_FRONT, INVERTERS_SIDE_LEFT), speed_threshold);
    double rpm_front_right = inverters_prv_avoid_zero_division(inverters_handler.get_rpm(INVERTERS_AXIS_FRONT, INVERTERS_SIDE_RIGHT), speed_threshold);
    double rpm_rear_left = inverters_prv_avoid_zero_division(inverters_handler.get_rpm(INVERTERS_AXIS_REAR, INVERTERS_SIDE_LEFT), speed_threshold);
    double rpm_rear_right = inverters_prv_avoid_zero_division(inverters_handler.get_rpm(INVERTERS_AXIS_REAR, INVERTERS_SIDE_RIGHT), speed_threshold);

    // --- FRONT LEFT PROCESS ---
    {
        double I_positive_limit = inverters_prv_compute_current_positive_limit(rpm_front_left, ratio_front_left);
        double I_negative_limit = inverters_prv_compute_current_negative_limit(rpm_front_left, ratio_front_left);

        if (I_positive_limit > 0.0F) {
            *torque_front_left_nm = fmin(*torque_front_left_nm, inverters_prv_current_to_torque(I_positive_limit));
        } else {
            *torque_front_left_nm = fmax(*torque_front_left_nm, inverters_prv_current_to_torque(I_positive_limit));
        }

        if (I_negative_limit > 0.0F) {
            *torque_front_left_nm = fmin(*torque_front_left_nm, inverters_prv_current_to_torque(I_negative_limit));
        } else {
            *torque_front_left_nm = fmax(*torque_front_left_nm, inverters_prv_current_to_torque(I_negative_limit));
        }
    }

    // --- FRONT RIGHT PROCESS ---
    {
        double I_positive_limit = inverters_prv_compute_current_positive_limit(rpm_front_right, ratio_front_right);
        double I_negative_limit = inverters_prv_compute_current_negative_limit(rpm_front_right, ratio_front_right);

        if (I_positive_limit > 0.0F) {
            *torque_front_right_nm = fmin(*torque_front_right_nm, inverters_prv_current_to_torque(I_positive_limit));
        } else {
            *torque_front_right_nm = fmax(*torque_front_right_nm, inverters_prv_current_to_torque(I_positive_limit));
        }

        if (I_negative_limit > 0.0F) {
            *torque_front_right_nm = fmin(*torque_front_right_nm, inverters_prv_current_to_torque(I_negative_limit));
        } else {
            *torque_front_right_nm = fmax(*torque_front_right_nm, inverters_prv_current_to_torque(I_negative_limit));
        }
    }

    // --- REAR LEFT PROCESS ---
    {
        double I_positive_limit = inverters_prv_compute_current_positive_limit(rpm_rear_left, ratio_rear_left);
        double I_negative_limit = inverters_prv_compute_current_negative_limit(rpm_rear_left, ratio_rear_left);

        if (I_positive_limit > 0.0F) {
            *torque_rear_left_nm = fmin(*torque_rear_left_nm, inverters_prv_current_to_torque(I_positive_limit));
        } else {
            *torque_rear_left_nm = fmax(*torque_rear_left_nm, inverters_prv_current_to_torque(I_positive_limit));
        }

        if (I_negative_limit > 0.0F) {
            *torque_rear_left_nm = fmin(*torque_rear_left_nm, inverters_prv_current_to_torque(I_negative_limit));
        } else {
            *torque_rear_left_nm = fmax(*torque_rear_left_nm, inverters_prv_current_to_torque(I_negative_limit));
        }
    }

    // --- REAR RIGHT PROCESS ---
    {
        double I_positive_limit = inverters_prv_compute_current_positive_limit(rpm_rear_right, ratio_rear_right);
        double I_negative_limit = inverters_prv_compute_current_negative_limit(rpm_rear_right, ratio_rear_right);

        if (I_positive_limit > 0.0F) {
            *torque_rear_right_nm = fmin(*torque_rear_right_nm, inverters_prv_current_to_torque(I_positive_limit));
        } else {
            *torque_rear_right_nm = fmax(*torque_rear_right_nm, inverters_prv_current_to_torque(I_positive_limit));
        }

        if (I_negative_limit > 0.0F) {
            *torque_rear_right_nm = fmin(*torque_rear_right_nm, inverters_prv_current_to_torque(I_negative_limit));
        } else {
            *torque_rear_right_nm = fmax(*torque_rear_right_nm, inverters_prv_current_to_torque(I_negative_limit));
        }
    }

    // Global protections
    inverters_prv_minimum_cell_voltage_limit(rpm_front_left, rpm_front_right, rpm_rear_left, rpm_rear_right, torque_front_left_nm, torque_front_right_nm, torque_rear_left_nm, torque_rear_right_nm);
    inverters_prv_maximum_allowable_power(rpm_front_left, rpm_front_right, rpm_rear_left, rpm_rear_right, torque_front_left_nm, torque_front_right_nm, torque_rear_left_nm, torque_rear_right_nm);
}

enum InvertersReturnCode inverters_api_init(
    inverters_send_drive_command_callback send_drive_command,
    inverters_set_torque_callback set_torque,
    inverters_get_rpm_callback get_rpm) {
    // validate callback pointers
    if ((send_drive_command == NULL) | (set_torque == NULL) | (get_rpm == NULL)) {
        return INVERTERS_RC_ERROR;
    }

    // store callbacks
    inverters_handler.send_drive_command = send_drive_command;
    inverters_handler.set_torque = set_torque;
    inverters_handler.get_rpm = get_rpm;

    // by default, disable all the inverters
    return inverters_api_set_drive(INVERTERS_DRIVE_STATUS_DISABLE);
}

enum InvertersReturnCode inverters_api_set_drive(enum InvertersDriveStatus drive_status) {
    if (inverters_handler.send_drive_command == NULL) {
        return INVERTERS_RC_ERROR;
    }

    // send command to all inverters
    // a single failure in the changing of state is not fatal for all the other inverters
    if ((inverters_handler.send_drive_command(drive_status, INVERTERS_AXIS_FRONT, INVERTERS_SIDE_LEFT) != INVERTERS_RC_OK) |
        (inverters_handler.send_drive_command(drive_status, INVERTERS_AXIS_FRONT, INVERTERS_SIDE_RIGHT) != INVERTERS_RC_OK) |
        (inverters_handler.send_drive_command(drive_status, INVERTERS_AXIS_REAR, INVERTERS_SIDE_LEFT) != INVERTERS_RC_OK) |
        (inverters_handler.send_drive_command(drive_status, INVERTERS_AXIS_REAR, INVERTERS_SIDE_RIGHT) != INVERTERS_RC_OK)) {
        return INVERTERS_RC_ERROR;
    }

    return INVERTERS_RC_OK;
}

enum InvertersReturnCode inverters_api_set_torque(double torque_front_left_nm, double torque_front_right_nm, double torque_rear_left_nm, double torque_rear_right_nm) {
    if (inverters_handler.set_torque == NULL) {
        return INVERTERS_RC_ERROR;
    }

    // apply cut off logic to make sure the requested torque doesn't damage the
    // battery pack and/or doesn't exceed the maximum power allowed by rule
    inverters_prv_apply_cut_off(&torque_front_left_nm, &torque_front_right_nm, &torque_rear_left_nm, &torque_rear_right_nm);

    /* Attempt to set torque for each inverter. 
     * If any single call fails, we stop and return an error to prevent 
     * asymmetric torque distribution across the axles.
     */
    if (inverters_handler.set_torque(torque_front_left_nm, INVERTERS_AXIS_FRONT, INVERTERS_SIDE_LEFT) != INVERTERS_RC_OK) {
        return INVERTERS_RC_ERROR;
    }

    if (inverters_handler.set_torque(torque_front_right_nm, INVERTERS_AXIS_FRONT, INVERTERS_SIDE_RIGHT) != INVERTERS_RC_OK) {
        return INVERTERS_RC_ERROR;
    }

    if (inverters_handler.set_torque(torque_rear_left_nm, INVERTERS_AXIS_REAR, INVERTERS_SIDE_LEFT) != INVERTERS_RC_OK) {
        return INVERTERS_RC_ERROR;
    }

    if (inverters_handler.set_torque(torque_rear_right_nm, INVERTERS_AXIS_REAR, INVERTERS_SIDE_RIGHT) != INVERTERS_RC_OK) {
        return INVERTERS_RC_ERROR;
    }

    return INVERTERS_RC_OK;
}