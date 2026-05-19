/*!
 * \file test_inverters.c
 * \author Dorijan Di Zepp
 * \date 2026-05-19
 * \brief Unit tests using FFF for testing the inverters module
 * 
 * \details This test suite uses 'extern' declarations to access internal 'static' functions 
 * from the \ref inverters.c module (specifically prv_inverters_apply_cut_off).
 * Internal functions are tested directly to verify the mathematical correctness 
 * of the power and torque limiting algorithms.
 */

#include <unity.h>
#include <stdbool.h>
#include <string.h>
#include "inverters-api.h"
#include "fff.h"
#include "eagletrt-api.h"

/* Declaration of the internal function for testing purposes */
extern void prv_inverters_apply_cut_off(float *torque_front_left_nm, float *torque_front_right_nm, float *torque_rear_left_nm, float *torque_rear_right_nm);

extern struct InvertersHandler inverters_handler;

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(enum InvertersReturnCode, send_drive_command, enum InvertersDriveStatus, enum InvertersPosition);
FAKE_VALUE_FUNC(enum InvertersReturnCode, set_torque, float, enum InvertersPosition);

void setUp(void) {
    // initialize inverters module
    inverters_api_init(send_drive_command, set_torque);

    // reset mock state
    RESET_FAKE(send_drive_command);
    RESET_FAKE(set_torque);

    FFF_RESET_HISTORY();
}

/* --- Test Cases --- */

/*!
 * \defgroup inverters_api_init Tests for inverters_api_init function
 * \{
 */

void test_inverters_api_init_null_send_drive_command(void) {
    // clear memory of actual inverters handler as already initialized in setUp
    memset(&inverters_handler, 0, sizeof(struct InvertersHandler));
    // generate expected handler state
    struct InvertersHandler expected_handler_state;
    memset(&expected_handler_state, 0, sizeof(struct InvertersHandler));

    enum InvertersReturnCode rc = inverters_api_init(NULL, set_torque);

    TEST_ASSERT_EQUAL_MESSAGE(INVERTERS_RC_ERROR, rc, "Init should fail if send_drive_command callback is NULL");
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&expected_handler_state, &inverters_handler, sizeof(struct InvertersHandler), "The handler internal memory has been changed");
}

void test_inverters_api_init_null_set_torque(void) {
    // clear memory of actual inverters handler as already initialized in setUp
    memset(&inverters_handler, 0, sizeof(struct InvertersHandler));
    // generate expected handler state
    struct InvertersHandler expected_handler_state;
    memset(&expected_handler_state, 0, sizeof(struct InvertersHandler));

    enum InvertersReturnCode rc = inverters_api_init(send_drive_command, NULL);

    TEST_ASSERT_EQUAL_MESSAGE(INVERTERS_RC_ERROR, rc, "Init should fail if set_torque callback is NULL");
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&expected_handler_state, &inverters_handler, sizeof(struct InvertersHandler), "The handler internal memory has been changed");
}

void test_inverters_api_init_failed_inverters_disabling(void) {
    // clear memory of actual inverters handler as already initialized in setUp
    memset(&inverters_handler, 0, sizeof(struct InvertersHandler));
    // generate expected handler state
    struct InvertersHandler expected_handler_state;
    memset(&expected_handler_state, 0, sizeof(struct InvertersHandler));
    expected_handler_state.send_drive_command = send_drive_command;
    expected_handler_state.set_torque = set_torque;
    memset(&(expected_handler_state.rpm_motors), 0, (sizeof(float) * INVERTERS_POSITION_COUNT));
    expected_handler_state.hv_bms_soc = 0.0F;

    send_drive_command_fake.return_val = INVERTERS_RC_ERROR;
    enum InvertersReturnCode rc = inverters_api_init(send_drive_command, set_torque);

    TEST_ASSERT_EQUAL_MESSAGE(INVERTERS_RC_ERROR, rc, "Init should fail if it failed to disable the inverters");
    // in this case because the set drive status failed, we still expected the handler
    // to be set with the callbacks
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&expected_handler_state, &inverters_handler, sizeof(struct InvertersHandler), "The handler internal memory has been changed");
}
/*! \} */

/*!
 * \defgroup inverters_api_set_drive Tests for inverters_api_set_drive function
 * \{
 */

void test_inverters_api_set_drive_unknown_drive_status(void) {
    enum InvertersReturnCode rc = inverters_api_set_drive_status(INVERTERS_DRIVE_STATUS_COUNT);

    TEST_ASSERT_EQUAL_MESSAGE(INVERTERS_RC_ERROR, rc, "Set drive should fail as the command passed is unknown");
}

void test_inverters_api_set_drive_failed_send_drive_command(void) {
    enum InvertersDriveStatus expected_drive_status = INVERTERS_DRIVE_STATUS_ENABLE;
    send_drive_command_fake.return_val = INVERTERS_RC_ERROR;

    enum InvertersReturnCode rc = inverters_api_set_drive_status(expected_drive_status);

    TEST_ASSERT_EQUAL_MESSAGE(expected_drive_status, send_drive_command_fake.arg0_val, "Set drive should receive the exact drive status indicated");
    TEST_ASSERT_EQUAL_MESSAGE(INVERTERS_RC_ERROR, rc, "Set drive should return error as at least one communication failed");
}

void test_inverters_api_set_drive_success_send_drive_command(void) {
    enum InvertersDriveStatus expected_drive_status = INVERTERS_DRIVE_STATUS_ENABLE;
    send_drive_command_fake.return_val = INVERTERS_RC_OK;

    enum InvertersReturnCode rc = inverters_api_set_drive_status(expected_drive_status);

    TEST_ASSERT_EQUAL_MESSAGE(expected_drive_status, send_drive_command_fake.arg0_val, "Set drive should receive the exact drive status indicated");
    TEST_ASSERT_EQUAL_MESSAGE(INVERTERS_RC_OK, rc, "Set drive should complete when all inverters receive the request");
}
/*! \} */

/*!
 * \defgroup inverters_api_set_rpm_motors Tests for function inverters_api_set_rpm_motors
 * \{
 */

void test_inverters_api_set_rpm_motors_null_array(void) {
    enum InvertersReturnCode rc = inverters_api_set_rpm_motors(NULL);

    TEST_ASSERT_EQUAL_MESSAGE(INVERTERS_RC_ERROR, rc, "Set rpm motors should fail when passing a NULL array");
}
/*! \} */

/*!
 * \defgroup inverters_api_set_hv_bms_soc Tests for function inverters_api_set_hv_bms_soc
 * \{
 */

void test_inverters_api_set_hv_bms_soc_lower_clamp(void) {
    float hv_bms_soc = -0.2F;
    float expected_hv_bms_soc = 0.0F;

    inverters_api_set_hv_bms_soc(hv_bms_soc);

    TEST_ASSERT_EQUAL_MESSAGE(expected_hv_bms_soc, inverters_handler.hv_bms_soc, "The soc setter should clamp to 0.0 if value passed is negative");
}

void test_inverters_api_set_hv_bms_soc_upper_clamp(void) {
    float hv_bms_soc = 1.2F;
    float expected_hv_bms_soc = 1.0F;

    inverters_api_set_hv_bms_soc(hv_bms_soc);

    TEST_ASSERT_EQUAL_MESSAGE(expected_hv_bms_soc, inverters_handler.hv_bms_soc, "The soc setter should clamp to 1.0 if value passed is greater than 1.0");
}
/*! \} */

/*!
 * \defgroup inverters_api_set_torque Tests for function prv_inverters_apply_cut_off
 * \{
 */

void test_inverters_api_set_torque_null_array(void) {
    enum InvertersReturnCode rc = inverters_api_set_torque(NULL);

    TEST_ASSERT_EQUAL_MESSAGE(INVERTERS_RC_ERROR, rc, "Set torque should fail when passing a NULL array");
}

void test_inverters_api_set_torque_never_exceeds_hardware_limits(void) {
    // Provide a request that is significantly higher than any physical limit
    float fl = 500.0F;
    float fr = 500.0F;
    float rl = 500.0F;
    float rr = 500.0F;

    // Use low RPM and high SoC to ensure we are testing the torque clamp
    // and not the power clamp or voltage sag logic.
    float rpm_motors[INVERTERS_POSITION_COUNT] = { 10.0F, 10.0F, 10.0F, 10.0F };
    inverters_api_set_rpm_motors(rpm_motors);
    inverters_api_set_hv_bms_soc(1.0F);

    // The logic should be bounded by the minimum of the motor peak and inverter peak
    float motor_ceiling = INVERTERS_MOTOR_PEAK_TORQUE_NM;
    float inverter_ceiling = INVERTERS_INVERTER_MAX_CONTINUOUS_CURRENT_A * INVERTERS_MOTOR_TORQUE_PER_CURRENT_NM_A;
    float absolute_max_allowed = EAGLETRT_API_MIN(motor_ceiling, inverter_ceiling);

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    TEST_ASSERT_LESS_OR_EQUAL_FLOAT_MESSAGE(absolute_max_allowed, fl, "Torque exceeded motor/inverter physical limits");
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT_MESSAGE(absolute_max_allowed, fr, "Torque exceeded motor/inverter physical limits");
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT_MESSAGE(absolute_max_allowed, rl, "Torque exceeded motor/inverter physical limits");
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT_MESSAGE(absolute_max_allowed, rr, "Torque exceeded motor/inverter physical limits");
}

void test_inverters_api_set_torque_never_exceeds_battery_max_power(void) {
    // Request maximum possible torque at a very high RPM (15,000)
    // This forces the total power to attempt to go way over 80kW.
    float fl = INVERTERS_MOTOR_PEAK_TORQUE_NM;
    float fr = INVERTERS_MOTOR_PEAK_TORQUE_NM;
    float rl = INVERTERS_MOTOR_PEAK_TORQUE_NM;
    float rr = INVERTERS_MOTOR_PEAK_TORQUE_NM;

    float test_rpm = 15000.0F;
    float rpm_motors[INVERTERS_POSITION_COUNT] = { test_rpm, test_rpm, test_rpm, test_rpm };
    inverters_api_set_rpm_motors(rpm_motors);
    inverters_api_set_hv_bms_soc(1.0F);

    const float omega = test_rpm * INVERTERS_RPM_TO_RAD_COEFFICIENT;

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    // The "invariant": P_total = (sum of torques) * omega <= BATTERY MAX POWER
    float total_torque = fl + fr + rl + rr;
    float actual_total_power = total_torque * omega;

    TEST_ASSERT_LESS_OR_EQUAL_FLOAT_MESSAGE(
        INVERTERS_HV_MAX_POWER_W,
        actual_total_power,
        "The logic failed to restrict total power to the legal limit");
}

void test_inverters_api_set_torque_no_cut_at_zero_rpm(void) {
    // Request a safe, mid-range torque.
    const float requested_torque = 10.0F;
    float fl = requested_torque, fr = requested_torque;
    float rl = requested_torque, rr = requested_torque;

    // Set RPM to exactly 0.0 to trigger the division-by-zero protection.
    float rpm_motors[INVERTERS_POSITION_COUNT] = { 0.0F };
    inverters_api_set_rpm_motors(rpm_motors);
    inverters_api_set_hv_bms_soc(1.0F);

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    // The result should be unmodified.
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.001F, requested_torque, fl, "Torque modified or corrupted by zero-division at 0 RPM");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.001F, requested_torque, fr, "Torque modified or corrupted by zero-division at 0 RPM");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.001F, requested_torque, rl, "Torque modified or corrupted by zero-division at 0 RPM");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.001F, requested_torque, rr, "Torque modified or corrupted by zero-division at 0 RPM");
}

void test_inverters_api_set_torque_preserves_ratio_during_cut(void) {
    // Request a specific ratio (e.g., 2:1)
    const float high_torque = 10.0f;
    const float low_torque = 5.0f;
    const float expected_ratio = high_torque / low_torque;

    float fl = high_torque, fr = low_torque;
    float rl = high_torque, rr = low_torque;

    float rpm_motors[INVERTERS_POSITION_COUNT] = { 5000.0F, 5000.0F, 5000.0F, 5000.0F };
    inverters_api_set_rpm_motors(rpm_motors);
    inverters_api_set_hv_bms_soc(1.0F);

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    // Ratio preservation
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.001f, expected_ratio, fl / fr, "Front Left/Right ratio was distorted during scaling");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.001f, expected_ratio, rl / rr, "Rear Left/Right ratio was distorted during scaling");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.001f, 1.0f, fl / rl, "Front/Rear balance was distorted during scaling");
}

void test_inverters_api_set_torque_preserves_ratio_at_high_rpm_saturation(void) {
    // At 14,000 RPM, the limit is roughly:
    // 20,000W / (14,000 * 0.1047 rad/s) = ~13.6 Nm
    float rpm_motors[INVERTERS_POSITION_COUNT] = { 14000.0F, 14000.0F, 14000.0F, 14000.0F };
    inverters_api_set_rpm_motors(rpm_motors);
    inverters_api_set_hv_bms_soc(1.0F);

    // We request 20Nm and 10Nm (2:1 ratio)
    // 20Nm is ABOVE the 13.6Nm limit.
    const float req_high = 20.0f;
    const float req_low = 10.0f;
    const float expected_ratio = 2.0f;

    float fl = req_high, fr = req_low;
    float rl = req_high, rr = req_low;

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    // Verify that a cut actually happened (fl should be capped at ~13.6Nm)
    TEST_ASSERT_TRUE_MESSAGE(fl < 14.0f, "Hardware limit was not triggered; fl is too high");

    // Verify ratio preservation
    // Even though 'fr' (10Nm) was technically safe, it should have been
    // scaled down to ~6.8Nm to maintain the 2:1 ratio with 'fl'.
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, expected_ratio, fl / fr, "Ratio distorted at saturation! Proportional scaling failed.");

    // Verify Front/Rear Balance
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 1.0f, fl / rl, "Front/Rear balance distorted at saturation!");
}

void test_inverters_api_set_torque_preserves_ratio_with_mixed_directions(void) {
    // Outside wheels pushing (+20), inside wheels braking (-10)
    float fl = 20.0f, fr = -10.0f;
    float rl = 20.0f, rr = -10.0f;
    // High speed to trigger power limit
    float rpm_motors[INVERTERS_POSITION_COUNT] = { 12000.0F, 12000.0F, 12000.0F, 12000.0F };
    inverters_api_set_rpm_motors(rpm_motors);

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    // The ratio between fl and fr must remain the same
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, -2.0f, fl / fr, "Mixed-direction ratio distorted");
}

void test_inverters_set_torque_regen_power_safety_limit(void) {
    // Request extreme regen (-88kW)
    float rpm_motors[INVERTERS_POSITION_COUNT] = { 10000.0F, 10000.0F, 10000.0F, 10000.0F };
    inverters_api_set_rpm_motors(rpm_motors);
    float fl = -21.0f, fr = -21.0f, rl = -21.0f, rr = -21.0f;
    float omega = 10000.0f * INVERTERS_RPM_TO_RAD_COEFFICIENT;

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    // Total power must not be "more negative" than the limit
    float actual_p = (fl + fr + rl + rr) * omega;
    TEST_ASSERT_TRUE_MESSAGE(actual_p >= INVERTERS_HV_MAX_REGEN_POWER_W - 1.0f,
                             "Regen power exceeded battery safety limits");
}

void test_inverters_set_torque_regen_preserves_ratio_during_cut(void) {
    // Request a 2:1 braking ratio
    float fl = -20.0f, fr = -10.0f;
    float rl = -20.0f, rr = -10.0f;
    const float expected_ratio = 2.0f;

    float rpm_motors[INVERTERS_POSITION_COUNT] = { 10000.0F, 10000.0F, 10000.0F, 10000.0F };
    inverters_api_set_rpm_motors(rpm_motors);

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    // Balance must remain 2:1
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.001f, expected_ratio, fl / fr, "L/R braking balance was distorted during regen scaling");
}

void test_inverters_set_torque_regen_actually_attenuates_torque(void) {
    const float requested = -21.0f;
    float fl = requested, fr = requested, rl = requested, rr = requested;

    float rpm_motors[INVERTERS_POSITION_COUNT] = { 10000.0F, 10000.0F, 10000.0F, 10000.0F };
    inverters_api_set_rpm_motors(rpm_motors);

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    // Torque must be "less negative" (closer to 0) than requested
    TEST_ASSERT_TRUE_MESSAGE(fl > requested,
                             "Regen torque was not attenuated despite exceeding power limits");
}

void test_inverters_api_set_torque_scales_on_low_voltage_sag(void) {
    // Request safe torque (10Nm) at high RPM (5000)
    float fl = 10.0f, fr = 10.0f, rl = 10.0f, rr = 10.0f;

    float rpm_motors[INVERTERS_POSITION_COUNT] = { 5000.0F, 5000.0F, 5000.0F, 5000.0F };
    inverters_api_set_rpm_motors(rpm_motors);
    // Simulate a nearly empty battery (SoC = 0.01)
    inverters_api_set_hv_bms_soc(0.01F);

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    // Even though 10Nm is safe hardware-wise,
    // it should be scaled down to protect against voltage collapse.
    TEST_ASSERT_TRUE_MESSAGE(fl < 10.0f, "Torque FL was not scaled down during extreme voltage sag");
    TEST_ASSERT_TRUE_MESSAGE(fr < 10.0f, "Torque FR was not scaled down during extreme voltage sag");
    TEST_ASSERT_TRUE_MESSAGE(rl < 10.0f, "Torque RL was not scaled down during extreme voltage sag");
    TEST_ASSERT_TRUE_MESSAGE(rr < 10.0f, "Torque RR was not scaled down during extreme voltage sag");
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, fl / fr); // Ratio must still hold!
}

void test_inverters_api_set_torque_reduces_drastically_at_zero_soc(void) {
    // Request high torque to force voltage sag given a SoC equal to zero
    float fl = 20.0f, fr = 20.0f, rl = 20.0f, rr = 20.0f;

    float rpm_motors[INVERTERS_POSITION_COUNT] = { 5000.0F, 5000.0F, 5000.0F, 5000.0F };
    inverters_api_set_rpm_motors(rpm_motors);
    inverters_api_set_hv_bms_soc(0.0F);

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    // Instead of 0, check that it's heavily attenuated (e.g., less than 5Nm)
    TEST_ASSERT_TRUE_MESSAGE(fl < 5.0f, "Torque FL was not sufficiently attenuated at 0% SoC");
    TEST_ASSERT_TRUE_MESSAGE(fr < 5.0f, "Torque FR was not sufficiently attenuated at 0% SoC");
    TEST_ASSERT_TRUE_MESSAGE(rl < 5.0f, "Torque RL was not sufficiently attenuated at 0% SoC");
    TEST_ASSERT_TRUE_MESSAGE(rr < 5.0f, "Torque RR was not sufficiently attenuated at 0% SoC");
}
/*! \} */

int main(void) {
    UNITY_BEGIN();

    /*!
     * \addtogroup inverters_api_init
     * \{
     */
    RUN_TEST(test_inverters_api_init_null_send_drive_command);
    RUN_TEST(test_inverters_api_init_null_set_torque);
    RUN_TEST(test_inverters_api_init_failed_inverters_disabling);
    /*! \} */

    /*!
     * \addtogroup inverters_api_set_drive
     * \{
     */
    RUN_TEST(test_inverters_api_set_drive_unknown_drive_status);
    RUN_TEST(test_inverters_api_set_drive_failed_send_drive_command);
    RUN_TEST(test_inverters_api_set_drive_success_send_drive_command);
    /*! \} */

    /*!
     * \addtogroup inverters_api_set_rpm_motors
     * \{
     */
    RUN_TEST(test_inverters_api_set_rpm_motors_null_array);
    /*! \} */

    /*!
     * \addtogroup inverters_api_set_hv_bms_soc
     * \{
     */
    RUN_TEST(test_inverters_api_set_hv_bms_soc_lower_clamp);
    RUN_TEST(test_inverters_api_set_hv_bms_soc_upper_clamp);
    /*! \} */

    /*!
     * \addtogroup inverters_api_set_torque
     * \{
     */
    RUN_TEST(test_inverters_api_set_torque_null_array);
    RUN_TEST(test_inverters_api_set_torque_never_exceeds_hardware_limits);
    RUN_TEST(test_inverters_api_set_torque_never_exceeds_battery_max_power);
    RUN_TEST(test_inverters_api_set_torque_no_cut_at_zero_rpm);
    RUN_TEST(test_inverters_api_set_torque_preserves_ratio_during_cut);
    RUN_TEST(test_inverters_api_set_torque_preserves_ratio_at_high_rpm_saturation);
    RUN_TEST(test_inverters_api_set_torque_preserves_ratio_with_mixed_directions);
    RUN_TEST(test_inverters_set_torque_regen_power_safety_limit);
    RUN_TEST(test_inverters_set_torque_regen_preserves_ratio_during_cut);
    RUN_TEST(test_inverters_set_torque_regen_actually_attenuates_torque);
    RUN_TEST(test_inverters_api_set_torque_scales_on_low_voltage_sag);
    RUN_TEST(test_inverters_api_set_torque_reduces_drastically_at_zero_soc);
    /*! \} */

    return UNITY_END();
}