/*!
 * \file test_inverters.c
 * \author Dorijan Di Zepp, Alessandro Bridi
 * \date 2026-07-31
 * \brief Unit tests for the inverters module and the ported Ephorus driver.
 */

#include <unity.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "inverters-api.h"
#include "ephorus-api.h"
#include "can-inverters-api.h"
#include "eagletrt-api.h"

extern void prv_inverters_apply_cut_off(float *torque_front_left_nm, float *torque_front_right_nm, float *torque_rear_left_nm, float *torque_rear_right_nm);
extern float prv_inverters_pack_voc_model(void);
extern float prv_inverters_internal_resistance_model(void);
extern struct InvertersHandler inverters_handler;

/*!
 * \brief Inject the same measured speed into every wheel's telemetry.
 *
 * \param rpm The speed to inject into every wheel's telemetry.
 */
static void set_all_rpm(int16_t rpm) {
    for (enum EphorusWheel wheel = 0; wheel < EPHORUS_WHEEL_COUNT; wheel++) {
        inverters_handler.driver.wheels[wheel].tlm.speed_rpm = rpm;
    }
}

void setUp(void) {
    inverters_api_init();
    inverters_api_attach(EPHORUS_WHEEL_FRONT_LEFT);
    inverters_api_attach(EPHORUS_WHEEL_FRONT_RIGHT);
    inverters_api_attach(EPHORUS_WHEEL_REAR_LEFT);
    inverters_api_attach(EPHORUS_WHEEL_REAR_RIGHT);
}

void tearDown(void) {
}

void test_cut_off_never_exceeds_hardware_limits(void) {
    // Request far above any physical limit, at low RPM and full SoC so the motor
    // torque clamp is what bounds the result.
    float fl = 500.0F, fr = 500.0F, rl = 500.0F, rr = 500.0F;
    set_all_rpm(10);
    inverters_api_set_soc(1.0F);

    float motor_ceiling = INVERTERS_MOTOR_PEAK_TORQUE_NM;
    float inverter_ceiling = INVERTERS_INVERTER_MAX_CONTINUOUS_CURRENT_A * INVERTERS_MOTOR_TORQUE_PER_CURRENT_NM_A;
    float absolute_max_allowed = EAGLETRT_API_MIN(motor_ceiling, inverter_ceiling);

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    TEST_ASSERT_LESS_OR_EQUAL_FLOAT_MESSAGE(absolute_max_allowed, fl, "Torque exceeded motor/inverter physical limits");
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT_MESSAGE(absolute_max_allowed, fr, "Torque exceeded motor/inverter physical limits");
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT_MESSAGE(absolute_max_allowed, rl, "Torque exceeded motor/inverter physical limits");
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT_MESSAGE(absolute_max_allowed, rr, "Torque exceeded motor/inverter physical limits");
}

void test_cut_off_never_exceeds_battery_max_power(void) {
    float fl = INVERTERS_MOTOR_PEAK_TORQUE_NM, fr = INVERTERS_MOTOR_PEAK_TORQUE_NM;
    float rl = INVERTERS_MOTOR_PEAK_TORQUE_NM, rr = INVERTERS_MOTOR_PEAK_TORQUE_NM;

    const int16_t test_rpm = 15000;
    set_all_rpm(test_rpm);
    inverters_api_set_soc(1.0F);

    const float omega = (float)test_rpm * INVERTERS_RPM_TO_RAD_COEFFICIENT;

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    float actual_total_power = (fl + fr + rl + rr) * omega;
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT_MESSAGE(INVERTERS_HV_MAX_POWER_W, actual_total_power, "The logic failed to restrict total power to the legal limit");
}

void test_cut_off_no_cut_at_zero_rpm(void) {
    const float requested_torque = 10.0F;
    float fl = requested_torque, fr = requested_torque, rl = requested_torque, rr = requested_torque;

    set_all_rpm(0);
    inverters_api_set_soc(1.0F);

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.001F, requested_torque, fl, "Torque modified or corrupted by zero-division at 0 RPM");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.001F, requested_torque, fr, "Torque modified or corrupted by zero-division at 0 RPM");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.001F, requested_torque, rl, "Torque modified or corrupted by zero-division at 0 RPM");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.001F, requested_torque, rr, "Torque modified or corrupted by zero-division at 0 RPM");
}

void test_cut_off_preserves_ratio_during_cut(void) {
    const float high_torque = 10.0f, low_torque = 5.0f;
    const float expected_ratio = high_torque / low_torque;

    float fl = high_torque, fr = low_torque, rl = high_torque, rr = low_torque;
    set_all_rpm(5000);
    inverters_api_set_soc(1.0F);

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.001f, expected_ratio, fl / fr, "Front Left/Right ratio was distorted during scaling");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.001f, expected_ratio, rl / rr, "Rear Left/Right ratio was distorted during scaling");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.001f, 1.0f, fl / rl, "Front/Rear balance was distorted during scaling");
}

void test_cut_off_preserves_ratio_at_high_rpm_saturation(void) {
    set_all_rpm(20000);
    inverters_api_set_soc(1.0F);

    const float req_high = 20.0f, req_low = 10.0f;
    const float expected_ratio = 2.0f;

    float fl = req_high, fr = req_low, rl = req_high, rr = req_low;

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    TEST_ASSERT_TRUE_MESSAGE(fl < 14.0f, "Hardware limit was not triggered; fl is too high");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, expected_ratio, fl / fr, "Ratio distorted at saturation! Proportional scaling failed.");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 1.0f, fl / rl, "Front/Rear balance distorted at saturation!");
}

void test_cut_off_preserves_ratio_with_mixed_directions(void) {
    float fl = 20.0f, fr = -10.0f, rl = 20.0f, rr = -10.0f;
    set_all_rpm(12000);
    inverters_api_set_soc(1.0F);

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, -2.0f, fl / fr, "Mixed-direction ratio distorted");
}

void test_cut_off_regen_power_safety_limit(void) {
    set_all_rpm(10000);
    inverters_api_set_soc(1.0F);
    float fl = -21.0f, fr = -21.0f, rl = -21.0f, rr = -21.0f;
    float omega = 10000.0f * INVERTERS_RPM_TO_RAD_COEFFICIENT;

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    float actual_p = (fl + fr + rl + rr) * omega;
    TEST_ASSERT_TRUE_MESSAGE(actual_p >= INVERTERS_HV_MAX_REGEN_POWER_W - 1.0f, "Regen power exceeded battery safety limits");
}

void test_cut_off_regen_preserves_ratio_during_cut(void) {
    float fl = -20.0f, fr = -10.0f, rl = -20.0f, rr = -10.0f;
    const float expected_ratio = 2.0f;
    set_all_rpm(10000);
    inverters_api_set_soc(1.0F);

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.001f, expected_ratio, fl / fr, "L/R braking balance was distorted during regen scaling");
}

void test_cut_off_regen_actually_attenuates_torque(void) {
    const float requested = -21.0f;
    float fl = requested, fr = requested, rl = requested, rr = requested;
    set_all_rpm(10000);
    inverters_api_set_soc(1.0F);

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    TEST_ASSERT_TRUE_MESSAGE(fl > requested, "Regen torque was not attenuated despite exceeding power limits");
}

void test_cut_off_scales_on_low_voltage_sag(void) {
    float fl = 10.0f, fr = 10.0f, rl = 10.0f, rr = 10.0f;
    set_all_rpm(5000);
    inverters_api_set_soc(0.01F);

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    TEST_ASSERT_TRUE_MESSAGE(fl < 10.0f, "Torque FL was not scaled down during extreme voltage sag");
    TEST_ASSERT_TRUE_MESSAGE(fr < 10.0f, "Torque FR was not scaled down during extreme voltage sag");
    TEST_ASSERT_TRUE_MESSAGE(rl < 10.0f, "Torque RL was not scaled down during extreme voltage sag");
    TEST_ASSERT_TRUE_MESSAGE(rr < 10.0f, "Torque RR was not scaled down during extreme voltage sag");
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, fl / fr);
}

void test_cut_off_reduces_drastically_at_zero_soc(void) {
    float fl = 20.0f, fr = 20.0f, rl = 20.0f, rr = 20.0f;
    set_all_rpm(5000);
    inverters_api_set_soc(0.0F);

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    TEST_ASSERT_TRUE_MESSAGE(fl < 5.0f, "Torque FL was not sufficiently attenuated at 0% SoC");
    TEST_ASSERT_TRUE_MESSAGE(fr < 5.0f, "Torque FR was not sufficiently attenuated at 0% SoC");
    TEST_ASSERT_TRUE_MESSAGE(rl < 5.0f, "Torque RL was not sufficiently attenuated at 0% SoC");
    TEST_ASSERT_TRUE_MESSAGE(rr < 5.0f, "Torque RR was not sufficiently attenuated at 0% SoC");
}

void test_set_soc_is_clamped_low(void) {
    inverters_api_set_soc(-0.2F);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0F, inverters_handler.hv_bms_soc, "SoC below range should clamp to 0.0");
}

void test_set_soc_is_clamped_high(void) {
    inverters_api_set_soc(1.5F);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(1.0F, inverters_handler.hv_bms_soc, "SoC above range should clamp to 1.0");
}

void test_voc_model_reference_values_0(void) {
    // Cell VOC(soc) = -3.85189120 s^4 + 9.42278296 s^3 - 8.31949326 s^2
    //               + 4.04805239 s + 2.82544823   (reference values below).
    inverters_api_set_soc(0.0F);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(1e-4f, 2.82544823f, prv_inverters_pack_voc_model(), "VOC(0%) should be the polynomial constant term");
}

void test_voc_model_reference_values_50(void) {
    inverters_api_set_soc(0.5F);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(1e-4f, 3.70670578f, prv_inverters_pack_voc_model(), "VOC(50%) mismatch (check the s^2 term sign)");
}

void test_voc_model_reference_values_100(void) {
    inverters_api_set_soc(1.0F);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(1e-4f, 4.12489912f, prv_inverters_pack_voc_model(), "VOC(100%) should be the sum of all coefficients");
}

void test_internal_resistance_model_reference_values_0(void) {
    // R_int(soc) = 0.0141 + 0.0021 * soc [Ohm].
    inverters_api_set_soc(0.0F);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(1e-6f, 0.0141f, prv_inverters_internal_resistance_model(), "R_int(0%) mismatch");
}

void test_internal_resistance_model_reference_values_100(void) {
    inverters_api_set_soc(1.0F);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(1e-6f, 0.0162f, prv_inverters_internal_resistance_model(), "R_int(100%) mismatch");
}

void test_physical_dc_current_limit_binds_below_80kw(void) {
    // At mid SoC the pack voltage is low enough that the 140 A DC-current limit
    // (pack_voltage * 140) falls BELOW the 80 kW regulatory ceiling, so it must
    // be the binding constraint. At 0.3 SoC: VOC ~= 3.5143 V/cell ->
    // pack ~= 506.06 V -> physical limit ~= 70.85 kW. This path is never reached
    // by the SoC = 1.0 tests, where 80 kW always dominates.
    set_all_rpm(12000);
    inverters_api_set_soc(0.3F);
    float fl = 21.0f, fr = 21.0f, rl = 21.0f, rr = 21.0f;

    const float omega = 12000.0f * INVERTERS_RPM_TO_RAD_COEFFICIENT;

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    float total_power = (fl + fr + rl + rr) * omega;
    TEST_ASSERT_TRUE_MESSAGE(total_power <= 72000.0f, "Total power must respect the ~70.85 kW DC-current limit, not the 80 kW ceiling");
    TEST_ASSERT_TRUE_MESSAGE(total_power > 60000.0f, "Limit should bind near the DC-current limit, not collapse torque");
}

void test_cut_off_leaves_within_limit_request_untouched(void) {
    // A modest, balanced request well inside every limit must pass through unscaled.
    float fl = 5.0f, fr = 5.0f, rl = 5.0f, rr = 5.0f;
    set_all_rpm(3000);
    inverters_api_set_soc(1.0F);

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.001f, 5.0f, fl, "Within-limit request must not be scaled (FL)");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.001f, 5.0f, fr, "Within-limit request must not be scaled (FR)");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.001f, 5.0f, rl, "Within-limit request must not be scaled (RL)");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.001f, 5.0f, rr, "Within-limit request must not be scaled (RR)");
}

/*!
 * \brief Arm + run a wheel, set its torque, build and decode its setpoint frame.
 *
 * \param wheel The wheel to arm and run.
 * \param nm The torque request in Nm.
 * \param armed Whether to arm the wheel before building the frame.
 *
 * \return The decoded setpoints message.
 */
static struct CanInvertersEphorusinverter1setpoints build_and_decode(enum EphorusWheel wheel, float nm, bool armed) {
    struct EphorusHandler *drv = &inverters_handler.driver;
    if (armed) {
        ephorus_api_arm(drv, wheel);
    }
    ephorus_api_set_torque(drv, wheel, nm);

    uint32_t id = 0;
    uint8_t data[EPHORUS_FRAME_DATA_SIZE] = { 0 };
    ephorus_api_build_setpoints(drv, wheel, &id, data);

    union CanInvertersMessages msg = { 0 };
    // done just because it's defined as [[nodiscard]]
    EAGLETRT_API_UNUSED(can_inverters_api_deserialize_from_id((enum CanInvertersMessageFrameId)id, data, &msg));
    return msg.ephorusinverter1setpoints;
}

void test_feature_positive_torque_sets_upper_bound_and_high_speed_rail(void) {
    struct CanInvertersEphorusinverter1setpoints s = build_and_decode(EPHORUS_WHEEL_FRONT_LEFT, 5.0f, true);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, s.enableinverter, "Armed + running wheel must be enabled");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 5.0f, s.torquelimitpositive, "Positive request => upper bound = request");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 0.0f, s.torquelimitnegative, "Positive request => lower bound = 0");
    TEST_ASSERT_EQUAL_INT16_MESSAGE(EPHORUS_DRIVE_SPEED_RPM, s.speedsetpoint, "Positive request => speed rail = 20000");
}

void test_feature_negative_torque_sets_lower_bound_and_zero_speed_rail(void) {
    struct CanInvertersEphorusinverter1setpoints s = build_and_decode(EPHORUS_WHEEL_REAR_RIGHT, -7.5f, true);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, s.enableinverter, "Armed + running wheel must be enabled");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 0.0f, s.torquelimitpositive, "Negative request => upper bound = 0");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, -7.5f, s.torquelimitnegative, "Negative request => lower bound = request");
    TEST_ASSERT_EQUAL_INT16_MESSAGE(0, s.speedsetpoint, "Negative request => speed rail = 0");
}

void test_feature_zero_torque_is_coast(void) {
    struct CanInvertersEphorusinverter1setpoints s = build_and_decode(EPHORUS_WHEEL_FRONT_RIGHT, 0.0f, true);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, s.enableinverter, "Even at 0 torque the armed + running wheel must be enabled");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 0.0f, s.torquelimitpositive, "Zero request => upper bound = 0");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 0.0f, s.torquelimitnegative, "Zero request => lower bound = 0");
    TEST_ASSERT_EQUAL_INT16_MESSAGE(EPHORUS_DRIVE_SPEED_RPM, s.speedsetpoint, "Zero request => speed rail = 20000");
}

void test_feature_disarmed_wheel_emits_zero_and_disabled(void) {
    // Not armed: even with a run request and a torque command the frame is inert.
    struct CanInvertersEphorusinverter1setpoints s = build_and_decode(EPHORUS_WHEEL_FRONT_LEFT, 10.0f, false);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, s.enableinverter, "Disarmed wheel must not enable the inverter");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 0.0f, s.torquelimitpositive, "Disarmed wheel => upper bound = 0");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 0.0f, s.torquelimitnegative, "Disarmed wheel => lower bound = 0");
    TEST_ASSERT_EQUAL_INT16_MESSAGE(EPHORUS_DRIVE_SPEED_RPM, s.speedsetpoint, "Disarmed wheel => speed rail = 20000");
}

void test_feature_torque_request_is_clamped(void) {
    struct CanInvertersEphorusinverter1setpoints s = build_and_decode(EPHORUS_WHEEL_FRONT_LEFT, 1000.0f, true);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, EPHORUS_MAX_TORQUE_NM, s.torquelimitpositive, "Torque request must clamp to EPHORUS_MAX_TORQUE_NM");
}

void test_on_receive_null_frame_is_rejected(void) {
    TEST_ASSERT_EQUAL(CAN_COMMUNICATION_RC_NULL_POINTER, inverters_api_on_receive(NULL));
}

int main(void) {
    UNITY_BEGIN();

    // Cut-off safety layer (preserved limits)
    RUN_TEST(test_cut_off_never_exceeds_hardware_limits);
    RUN_TEST(test_cut_off_never_exceeds_battery_max_power);
    RUN_TEST(test_cut_off_no_cut_at_zero_rpm);
    RUN_TEST(test_cut_off_preserves_ratio_during_cut);
    RUN_TEST(test_cut_off_preserves_ratio_at_high_rpm_saturation);
    RUN_TEST(test_cut_off_preserves_ratio_with_mixed_directions);
    RUN_TEST(test_cut_off_regen_power_safety_limit);
    RUN_TEST(test_cut_off_regen_preserves_ratio_during_cut);
    RUN_TEST(test_cut_off_regen_actually_attenuates_torque);
    RUN_TEST(test_cut_off_scales_on_low_voltage_sag);
    RUN_TEST(test_cut_off_reduces_drastically_at_zero_soc);
    RUN_TEST(test_set_soc_is_clamped_low);
    RUN_TEST(test_set_soc_is_clamped_high);

    // Battery models + limit-path coverage
    RUN_TEST(test_voc_model_reference_values_0);
    RUN_TEST(test_voc_model_reference_values_50);
    RUN_TEST(test_voc_model_reference_values_100);
    RUN_TEST(test_internal_resistance_model_reference_values_0);
    RUN_TEST(test_internal_resistance_model_reference_values_100);
    RUN_TEST(test_physical_dc_current_limit_binds_below_80kw);
    RUN_TEST(test_cut_off_leaves_within_limit_request_untouched);

    // Torque-control-via-speed-rail feature
    RUN_TEST(test_feature_positive_torque_sets_upper_bound_and_high_speed_rail);
    RUN_TEST(test_feature_negative_torque_sets_lower_bound_and_zero_speed_rail);
    RUN_TEST(test_feature_zero_torque_is_coast);
    RUN_TEST(test_feature_disarmed_wheel_emits_zero_and_disabled);
    RUN_TEST(test_feature_torque_request_is_clamped);

    // RX telemetry decode + fault latching
    RUN_TEST(test_on_receive_null_frame_is_rejected);

    return UNITY_END();
}
