/*!
 * \file test_inverters.c
 * \author Dorijan Di Zepp
 * \date 2026-05-09
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

void setUp(void) {
    // initialize inverters module
    inverters_api_init(send_drive_command, set_torque, get_rpm);

    // reset mock state
    RESET_FAKE(send_drive_command);
    RESET_FAKE(set_torque);
    RESET_FAKE(get_rpm);

    FFF_RESET_HISTORY();
}

/* --- Test Cases --- */

/*!
 * \defgroup inverters_api_init Tests for inverters_api_init function
 * \{
 */

void test_inverters_api_init_null_send_drive_command(void) {
    enum InvertersReturnCode rc = inverters_api_init(NULL, set_torque, get_rpm);

    TEST_ASSERT_EQUAL_MESSAGE(INVERTERS_RC_ERROR, rc, "Init should fail if send_drive_command callback is NULL");
}

void test_inverters_api_init_null_set_torque(void) {
    enum InvertersReturnCode rc = inverters_api_init(send_drive_command, NULL, get_rpm);

    TEST_ASSERT_EQUAL_MESSAGE(INVERTERS_RC_ERROR, rc, "Init should fail if set_torque callback is NULL");
}

void test_inverters_api_init_null_get_rpm(void) {
    enum InvertersReturnCode rc = inverters_api_init(send_drive_command, set_torque, NULL);

    TEST_ASSERT_EQUAL_MESSAGE(INVERTERS_RC_ERROR, rc, "Init should fail if get_rpm callback is NULL");
}

void test_inverters_api_init_failed_inverters_disabling(void) {
    send_drive_command_fake.return_val = INVERTERS_RC_ERROR;
    enum InvertersReturnCode rc = inverters_api_init(send_drive_command, set_torque, get_rpm);

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
 * \defgroup inverters_api_set_torque Tests for inverters_api_set_torque function
 * \{
 */

void test_inverters_api_set_torque_clamps_to_mechanical_peak(void) {
    float fl = 50.0f, fr = 50.0f, rl = 50.0f, rr = 50.0f;
    get_rpm_fake.return_val = 1000.0f;

    // Call the logic function directly with pointers
    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    // Now fl, fr, rl, rr have been modified by the function
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(MOTOR_PEAK_TORQUE_NM, fl, "Front left torque not clamped to mechanical peak");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(MOTOR_PEAK_TORQUE_NM, fr, "Front right torque not clamped to mechanical peak");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(MOTOR_PEAK_TORQUE_NM, rl, "Rear left torque not clamped to mechanical peak");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(MOTOR_PEAK_TORQUE_NM, rr, "Rear right torque not clamped to mechanical peak");
}

void test_inverters_api_set_torque_limits_to_80kw(void) {
    // Requesting 21Nm per motor at 15,000 RPM.
    // Total power = 4 * 21Nm * (15000 * PI/30) = ~132 kW (Illegal)
    float fl = 21.0f, fr = 21.0f, rl = 21.0f, rr = 21.0f;
    get_rpm_fake.return_val = 15000.0f;

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    float total_torque = fl + fr + rl + rr;
    float omega = 15000.0f * RPM_TO_RAD_COEFF;
    float total_power = total_torque * omega;

    TEST_ASSERT_TRUE_MESSAGE(total_power <= BATTERY_MAX_POWER_W, "Total power exceeded 80kW limit");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(10.0f, BATTERY_MAX_POWER_W, total_power, "Power not correctly scaled to 80kW");
}

void test_inverters_api_set_torque_scales_proportionally(void) {
    // Front 10Nm, Rear 20Nm (1:2 ratio). At 15k RPM, this exceeds 80kW.
    float fl = 10.0f, fr = 10.0f, rl = 20.0f, rr = 20.0f;
    float expected_ratio = 10.0f / 20.0f;
    get_rpm_fake.return_val = 15000.0f;

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    float actual_ratio = fl / rl;
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, expected_ratio, actual_ratio, "Torque distribution ratio was not preserved during scaling");
}

void test_inverters_api_set_torque_limits_regen_power(void) {
    // Requesting heavy braking (-21Nm) at 8000 RPM.
    float fl = -21.0f, fr = -21.0f, rl = -21.0f, rr = -21.0f;
    get_rpm_fake.return_val = 8000.0f;

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    float total_regen_power = (fl + fr + rl + rr) * (8000.0f * RPM_TO_RAD_COEFF);

    // Power is negative; verify it is not "more negative" than the limit.
    TEST_ASSERT_TRUE_MESSAGE(total_regen_power >= BATTERY_MAX_REGEN_POWER_W, "Regenerative power exceeded battery charging limits");
}

void test_inverters_api_set_torque_no_cut_at_zero_rpm(void) {
    // Requesting legal torque at standstill.
    float fl = 10.0f, fr = 10.0f, rl = 10.0f, rr = 10.0f;
    get_rpm_fake.return_val = 0.0f;

    prv_inverters_apply_cut_off(&fl, &fr, &rl, &rr);

    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(10.0f, fl, "Torque was unexpectedly modified at zero RPM");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(10.0f, rr, "Torque was unexpectedly modified at zero RPM");
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
    RUN_TEST(test_inverters_api_set_torque_limits_to_80kw);
    RUN_TEST(test_inverters_api_set_torque_scales_proportionally);
    RUN_TEST(test_inverters_api_set_torque_limits_regen_power);
    RUN_TEST(test_inverters_api_set_torque_no_cut_at_zero_rpm);
    /*! \} */

    return UNITY_END();
}