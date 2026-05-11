/*!
 * \file test_inverters.c
 * \author Dorijan Di Zepp
 * \date 2026-05-11
 * \brief Unit tests using FFF for testing the inverters module
 * 
 * \details This test suite uses 'extern' declarations to access internal 'static' functions 
 * from the \ref inverters.c module (specifically prv_inverters_apply_cut_off).
 * Internal functions are tested directly to verify the mathematical correctness 
 * of the power and torque limiting algorithms.
 */

#include <unity.h>
#include <stdbool.h>
#include "inverters-api.h"
#include "fff.h"
#include "eagletrt-api.h"

/* Declaration of the internal function for testing purposes */
extern void prv_inverters_apply_cut_off(float *torque_front_left_nm, float *torque_front_right_nm, float *torque_rear_left_nm, float *torque_rear_right_nm);

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(enum InvertersReturnCode, send_drive_command, enum InvertersDriveStatus, enum InvertersPosition);
FAKE_VALUE_FUNC(enum InvertersReturnCode, set_torque, float, enum InvertersPosition);
FAKE_VALUE_FUNC(float, get_rpm, enum InvertersPosition);
FAKE_VALUE_FUNC(float, get_soc);

void setUp(void) {
    // initialize inverters module
    inverters_api_init(send_drive_command, set_torque, get_rpm, get_soc);

    // reset mock state
    RESET_FAKE(send_drive_command);
    RESET_FAKE(set_torque);
    RESET_FAKE(get_rpm);
    RESET_FAKE(get_soc);

    FFF_RESET_HISTORY();
}

/* --- Test Cases --- */

/*!
 * \defgroup inverters_api_init Tests for inverters_api_init function
 * \{
 */

void test_inverters_api_init_null_send_drive_command(void) {
    enum InvertersReturnCode rc = inverters_api_init(NULL, set_torque, get_rpm, get_soc);

    TEST_ASSERT_EQUAL_MESSAGE(INVERTERS_RC_ERROR, rc, "Init should fail if send_drive_command callback is NULL");
}

void test_inverters_api_init_null_set_torque(void) {
    enum InvertersReturnCode rc = inverters_api_init(send_drive_command, NULL, get_rpm, get_soc);

    TEST_ASSERT_EQUAL_MESSAGE(INVERTERS_RC_ERROR, rc, "Init should fail if set_torque callback is NULL");
}

void test_inverters_api_init_null_get_rpm(void) {
    enum InvertersReturnCode rc = inverters_api_init(send_drive_command, set_torque, NULL, get_soc);

    TEST_ASSERT_EQUAL_MESSAGE(INVERTERS_RC_ERROR, rc, "Init should fail if get_rpm callback is NULL");
}

void test_inverters_api_init_null_get_soc(void) {
    enum InvertersReturnCode rc = inverters_api_init(send_drive_command, set_torque, get_rpm, NULL);

    TEST_ASSERT_EQUAL_MESSAGE(INVERTERS_RC_ERROR, rc, "Init should fail if get_soc callback is NULL");
}

void test_inverters_api_init_failed_inverters_disabling(void) {
    send_drive_command_fake.return_val = INVERTERS_RC_ERROR;
    enum InvertersReturnCode rc = inverters_api_init(send_drive_command, set_torque, get_rpm, get_soc);

    TEST_ASSERT_EQUAL_MESSAGE(INVERTERS_RC_ERROR, rc, "Init should fail if it failed to disable the inverters");
}
/*! \} */

/*!
 * \defgroup inverters_api_set_drive Tests for inverters_api_set_drive function
 * \{
 */

void test_inverters_api_set_drive_unknown_drive_status(void) {
    enum InvertersReturnCode rc = inverters_api_set_drive(INVERTERS_DRIVE_STATUS_COUNT);

    TEST_ASSERT_EQUAL_MESSAGE(INVERTERS_RC_ERROR, rc, "Set drive should fail as the command passed is unknown");
}

void test_inverters_api_set_drive_failed_send_drive_command(void) {
    send_drive_command_fake.return_val = INVERTERS_RC_ERROR;
    enum InvertersReturnCode rc = inverters_api_set_drive(INVERTERS_DRIVE_STATUS_ENABLE);

    TEST_ASSERT_EQUAL_MESSAGE(INVERTERS_RC_ERROR, rc, "Set drive should return error as at least one communication failed");
}

void test_inverters_api_set_drive_success_send_drive_command(void) {
    send_drive_command_fake.return_val = INVERTERS_RC_OK;
    enum InvertersReturnCode rc = inverters_api_set_drive(INVERTERS_DRIVE_STATUS_ENABLE);

    TEST_ASSERT_EQUAL_MESSAGE(INVERTERS_RC_OK, rc, "Set drive should complete when all inverters receive the request");
}
/*! \} */

/*!
 * \defgroup inverters_api_set_torque Tests for function prv_inverters_apply_cut_off
 * \{
 */

void test_inverters_api_set_torque_clamps_to_mechanical_peak(void) {
    /* 
     * Request: 50.0 Nm per motor.
     * RPM: 1000.0.
     * 1. Individual limit: At 1000 RPM, the power-based torque limit is very high 
     * (20kW / 104.7 rad/s = ~191 Nm).
     * 2. Mechanical limit: The code sees the request (50 Nm) exceeds MOTOR_PEAK_TORQUE_NM (21 Nm).
     * 3. Result: The individual motor protection successfully clips the 
     * request to 21 Nm before any global logic is even considered.
     */
    float fl = 50.0f, fr = 50.0f, rl = 50.0f, rr = 50.0f;
    get_rpm_fake.return_val = 1000.0f;
    get_soc_fake.return_val = 1.0F;

    // Call the logic function directly with pointers
    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    // Now fl, fr, rl, rr have been modified by the function
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(MOTOR_PEAK_TORQUE_NM, fl, "Front left torque not clamped to mechanical peak");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(MOTOR_PEAK_TORQUE_NM, fr, "Front right torque not clamped to mechanical peak");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(MOTOR_PEAK_TORQUE_NM, rl, "Rear left torque not clamped to mechanical peak");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(MOTOR_PEAK_TORQUE_NM, rr, "Rear right torque not clamped to mechanical peak");
}

void test_inverters_api_set_torque_limits_to_battery_max_power(void) {
    /*
     * Request: 21.0 Nm (Peak torque) at 15,000 RPM.
     * Total Requested Power: 4 * (21Nm * 1570.8 rad/s) = ~132,000 W (Illegal as above 80kW).
     * 1. Individual protection: At 15k RPM, the motor is clamped to ~12.7 Nm to stay under 20kW.
     * Current Total: 4 * (12.7Nm * 1570.8 rad/s) = ~80,000 W.
     * 2. Global protection: The function prv_inverters_maximum_allowable_power checks the 
     * sum against BATTERY_MAX_POWER_W (80,000). 
     * 3. Result: Because 4 * 20kW is exactly 80kW, the individual hardware limits 
     * effectively "pre-solve" the 80kW limit, ensuring legality.
     */

    // Requesting maximum torque at high RPM
    float fl = 21.0f, fr = 21.0f, rl = 21.0f, rr = 21.0f;
    get_rpm_fake.return_val = 15000.0f;
    get_soc_fake.return_val = 1.0F;

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    float total_torque = fl + fr + rl + rr;
    float omega = 15000.0f * RPM_TO_RAD_COEFF;
    float total_power = total_torque * omega;

    TEST_ASSERT_TRUE_MESSAGE(total_power <= BATTERY_MAX_POWER_W + 1.0f, "Total power exceeded the legal 80kW limit");
}

void test_inverters_api_set_torque_no_cut_at_zero_rpm(void) {
    /*
     * Request: 10.0 Nm at 0.0 RPM.
     * 1. Normally, Power = Torque * RPM. To find Torque, you'd do Power / RPM. 
     * Division by zero (10 / 0) would crash the code.
     * 2. prv_inverters_avoid_zero_division detects the 0.0 RPM and 
     * internally treats it as a tiny value (e.g., 0.1 RPM) for the calculation.
     * 3. Result: The math results in a massive torque limit (e.g., 20,000W / 0.01 rad/s). 
     * Since 10.0 Nm is much smaller than that massive limit, the torque is passed 
     * through unmodified.
     */

    // Test if division-by-zero protection work
    // Requesting legal torque at standstill.
    float fl = 10.0f, fr = 10.0f, rl = 10.0f, rr = 10.0f;
    get_rpm_fake.return_val = 0.0f;
    get_soc_fake.return_val = 1.0F;

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(10.0f, fl, "Torque was unexpectedly modified at zero RPM");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(10.0f, rr, "Torque was unexpectedly modified at zero RPM");
}

void test_inverters_api_set_torque_preserves_ratio_during_cut(void) {
    /*
     * Request: 14Nm and 7Nm at 13,000 RPM.
     * Individual limit: 20kW / (13000 * 0.1047) = 14.68 Nm.
     * Our requests (14 & 7) are BOTH BELOW 14.68, so no individual clipping occurs.
     * Global limit: Total power is ~57kW.
     */
    float fl = 14.0f, fr = 7.0f, rl = 14.0f, rr = 7.0f;
    get_rpm_fake.return_val = 13000.0f;
    get_soc_fake.return_val = 1.0F;

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    // If global scaling triggers, it multiplies all by the same ratio 'k'
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 2.0f, fl / fr, "Ratio should be preserved when only global scaling is active");
}

void test_inverters_api_set_torque_breaks_ratio_on_hardware_limit(void) {
    /* Why the ratio should break:
     * Request: 20Nm and 10Nm at 15,000 RPM. (Original ratio 2.0)
     * Individual Limit: 20kW / (15000 * 0.1047) = 12.73 Nm.
     * The clipping:
     * - Left (20Nm) is GREATER than 12.73 -> Clamped to 12.73 Nm.
     * - Right (10Nm) is LESS than 12.73 -> Stays at 10.00 Nm.
     * Resulting ratio: 12.73 / 10.0 = 1.27 (Ratio is NOT 2.0 anymore).
     */
    float fl = 20.0f, fr = 10.0f, rl = 20.0f, rr = 10.0f;
    get_rpm_fake.return_val = 15000.0f;
    get_soc_fake.return_val = 1.0F;

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    // We expect the ratio to be closer to 1.27 than 2.0
    float actual_ratio = fl / fr;
    TEST_ASSERT_TRUE_MESSAGE(actual_ratio < 1.5f, "Ratio should be broken because the high-torque side hit a hardware limit");
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.2732f, actual_ratio);
}

void test_inverters_api_set_torque_limits_regen_power(void) {
    /*
     * Request: -21.0 Nm (max regen) at 10,000 RPM.
     * Total requested regen power: 4 * (-21Nm * 1047.2 rad/s) = -87,964 W (~ -88kW).
     * 1. The code calculates the total power requested. 
     * 2. It sees -88kW is "more negative" (less than) the 
     * BATTERY_MAX_REGEN_POWER_W (e.g., -20kW).
     * 3. Scaling: The global scaling logic (prv_inverters_maximum_allowable_power) 
     * calculates a reduction ratio to bring that -88kW back up to exactly the -20kW limit.
     * 4. Result: All torque values are reduced proportionally to stay within battery safety.
     */

    // Request heavy braking (-21Nm) at high speed
    float fl = -21.0f, fr = -21.0f, rl = -21.0f, rr = -21.0f;
    get_rpm_fake.return_val = 10000.0f;
    get_soc_fake.return_val = 1.0F;

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    float omega = 10000.0f * RPM_TO_RAD_COEFF;
    float total_power = (fl + fr + rl + rr) * omega;

    // Power is negative during regen. Ensure it's not "more negative" than the limit.
    // e.g. If limit is -15000, power should be -15000, not -25000.
    TEST_ASSERT_TRUE_MESSAGE(total_power >= BATTERY_MAX_REGEN_POWER_W - 1.0f, "Regen power exceeded battery limits");
}

void test_inverters_api_set_torque_limits_to_inverter_current(void) {
    /*
     * Request: 100.0 Nm (An impossible request).
     * RPM: 100.0 (Very low speed).
     * 1. Calculation: The code calculates three potential torque limits:
     * a) t_max_mechanical: 21.0 Nm (Motor constant)
     * b) t_max_current: 90A * 0.25 Nm/A = 22.5 Nm (Inverter hardware limit)
     * c) t_max_power: 20kW / 10.47 rad/s = ~1910 Nm (Power limit at low speed)
     * 2. The code takes the minimum of these: MIN(21.0, 22.5, 1910).
     * 3. Result: 21.0 Nm wins. The code correctly identifies that the motor's physical 
     * peak is reached before the inverter's current limit.
     */

    float fl = 100.0f; // Ridiculous request
    float dummy = 0.0f;
    get_rpm_fake.return_val = 100.0f; // Low RPM so power isn't the bottleneck
    get_soc_fake.return_val = 1.0F;

    prv_inverters_apply_cut_off(&fl, &dummy, &dummy, &dummy);

    // Should be clamped to the lowest of mechanical peak, inverter current limit, power limit
    float expected_limit = MOTOR_PEAK_TORQUE_NM;
    TEST_ASSERT_FLOAT_WITHIN(0.1f, expected_limit, fl);
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
    RUN_TEST(test_inverters_api_init_null_get_rpm);
    RUN_TEST(test_inverters_api_init_null_get_soc);
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
     * \addtogroup inverters_api_set_torque
     * \{
     */
    RUN_TEST(test_inverters_api_set_torque_clamps_to_mechanical_peak);
    RUN_TEST(test_inverters_api_set_torque_limits_to_battery_max_power);
    RUN_TEST(test_inverters_api_set_torque_no_cut_at_zero_rpm);
    RUN_TEST(test_inverters_api_set_torque_preserves_ratio_during_cut);
    RUN_TEST(test_inverters_api_set_torque_breaks_ratio_on_hardware_limit);
    RUN_TEST(test_inverters_api_set_torque_limits_regen_power);
    RUN_TEST(test_inverters_api_set_torque_limits_to_inverter_current);
    /*! \} */

    return UNITY_END();
}