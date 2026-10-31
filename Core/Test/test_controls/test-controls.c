/*!
 * \file test-controls.c
 * \date 2026-10-27
 * \authors Thomas Moretti [majorfrost173@gmail.com]
 *
 * \brief Tests for the controls module.
 *
 * \details The tests try to follow the arrange -> act -> assert scheme.
 */

#include "controls-api.h"
#include "fff.h"
#include <unity.h>

DEFINE_FFF_GLOBALS;

extern struct ControlsHandler controls_handler;

FAKE_VOID_FUNC(mock_controls_send, const enum SpiNetwork, const uint8_t *const, const size_t);
FAKE_VOID_FUNC(mock_controls_send_receive, const enum SpiNetwork, const uint8_t *const, uint8_t *const, const size_t, const size_t);

void setUp(void) {
    RESET_FAKE(mock_controls_send);
    RESET_FAKE(mock_controls_send_receive);
    FFF_RESET_HISTORY();

    controls_api_init(mock_controls_send, mock_controls_send_receive);
}

void tearDown(void) {
}

void test_controls_api_init_fails_on_null_config(void) {
    memset(&controls_handler, 0U, sizeof(struct ControlsHandler));
    enum ControlsReturnCode rc = controls_api_init(NULL, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(rc, CONTROLS_RC_NULL_POINTER, "Totally invalid configuration should make the function return a null pointer code");
}

void test_controls_api_init_fails_on_any_null_callback(void) {
    memset(&controls_handler, 0U, sizeof(struct ControlsHandler));
    enum ControlsReturnCode rc = controls_api_init(mock_controls_send, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(rc, CONTROLS_RC_NULL_POINTER, "Null callback should make the function return a null pointer code");
}

void test_controls_api_init_works_with_correct_parameters() {
    enum ControlsReturnCode rc = controls_api_init(mock_controls_send, mock_controls_send_receive);
    TEST_ASSERT_EQUAL_MESSAGE(rc, CONTROLS_RC_OK, "Valid configuration should make the function return an ok code");
}

void test_controls_api_get_control_value_invalid_control(void) {
    TEST_ASSERT_EQUAL_MESSAGE(controls_api_get_control_value(CONTROLS_NAME_COUNT), 0U, "Invalid controls should make the function return 0");
}

void test_controls_api_get_control_value_valid_control(void) {
    constexpr uint8_t fake_level = 9U;
    controls_handler.values[CONTROLS_NAME_POWER] = fake_level;
    TEST_ASSERT_EQUAL_MESSAGE(controls_api_get_control_value(CONTROLS_NAME_POWER), fake_level, "A call to the function that provides a valid control should return the correct value associated to that control");
}

void test_controls_api_set_control_value_invalid_control(void) {
    TEST_ASSERT_EQUAL_MESSAGE(controls_api_set_control_value(CONTROLS_NAME_COUNT, 123U), CONTROLS_RC_ERROR, "Invalid controls should make the function return an error code");
}

void test_controls_api_set_control_value_level_valid_control_should_get_clamped_upper(void) {
    TEST_ASSERT_EQUAL_MESSAGE(controls_api_set_control_value(CONTROLS_NAME_REGEN, controls_api_max_level + 1U), CONTROLS_RC_OK, "Valid controls should make the function return an ok code");
    TEST_ASSERT_EQUAL_MESSAGE(controls_handler.values[CONTROLS_NAME_REGEN], controls_api_max_level, "Calling set with a value over the range should have it clamped down");
}

void test_controls_api_set_control_value_launch_control_should_get_clamped_to_one(void) {
    TEST_ASSERT_EQUAL_MESSAGE(controls_api_set_control_value(CONTROLS_NAME_LAUNCH_CONTROL, 2U), CONTROLS_RC_OK, "Valid controls should make the function return an ok code");
    TEST_ASSERT_EQUAL_MESSAGE(controls_handler.values[CONTROLS_NAME_LAUNCH_CONTROL], 1U, "Calling set with a value over the range should have it clamped down");
}

void test_controls_api_get_torque_invalid_wheel(void) {
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(controls_api_get_torque(EPHORUS_WHEEL_COUNT), .0F, "Invalid wheel should make the function return 0.0 (approximately)");
}

void test_controls_api_get_torque_valid_wheel(void) {
    constexpr float fake_torque = 23.6F;
    controls_handler.requested_torque_nm[EPHORUS_WHEEL_REAR_LEFT] = fake_torque;
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(controls_api_get_torque(EPHORUS_WHEEL_REAR_LEFT), fake_torque, "Valid wheel should make the function return the corresponding torque");
}

void test_controls_api_set_torque_invalid_wheel(void) {
    TEST_ASSERT_EQUAL_MESSAGE(controls_api_set_torque(EPHORUS_WHEEL_COUNT, 34.3F), CONTROLS_RC_ERROR, "Invalid wheel should make the function return an error code");
}

void test_controls_api_set_torque_valid_wheel(void) {
    constexpr float fake_torque = 41.9F;
    TEST_ASSERT_EQUAL_MESSAGE(controls_api_set_torque(EPHORUS_WHEEL_FRONT_RIGHT, fake_torque), CONTROLS_RC_OK, "Valid wheel should make the function return an ok code");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(controls_handler.requested_torque_nm[EPHORUS_WHEEL_FRONT_RIGHT], fake_torque, "Calling set with a certain value should save it as is");
}

void test_controls_api_get_status_works_properly(void) {
    constexpr enum ControlsStatus fake_status = CONTROLS_STATUS_OK;
    controls_handler.status = fake_status;
    TEST_ASSERT_EQUAL_MESSAGE(controls_api_get_status(), fake_status, "The function should always return the same status that is saved in the handler");
}

void test_controls_api_set_status_invalid_status(void) {
    TEST_ASSERT_EQUAL_MESSAGE(controls_api_set_status(CONTROLS_STATUS_COUNT), CONTROLS_RC_ERROR, "Invalid status should make the function return an error code");
}

void test_controls_api_set_status_valid_status(void) {
    constexpr enum ControlsStatus fake_status = CONTROLS_STATUS_ERROR;
    TEST_ASSERT_EQUAL_MESSAGE(controls_api_set_status(fake_status), CONTROLS_RC_OK, "Valid status should make the function return an ok code");
    TEST_ASSERT_EQUAL_MESSAGE(controls_handler.status, fake_status, "Calling set with a valid status should save it as is");
}

int main(void) {
    UNITY_BEGIN();

    // test controls_api_init
    RUN_TEST(test_controls_api_init_fails_on_null_config);
    RUN_TEST(test_controls_api_init_fails_on_any_null_callback);
    RUN_TEST(test_controls_api_init_works_with_correct_parameters);

    // test controls_api_get_control_value
    RUN_TEST(test_controls_api_get_control_value_invalid_control);
    RUN_TEST(test_controls_api_get_control_value_valid_control);

    // test controls_api_set_control_value
    RUN_TEST(test_controls_api_set_control_value_invalid_control);
    RUN_TEST(test_controls_api_set_control_value_level_valid_control_should_get_clamped_upper);
    RUN_TEST(test_controls_api_set_control_value_launch_control_should_get_clamped_to_one);

    // test controls_api_get_torque
    RUN_TEST(test_controls_api_get_torque_invalid_wheel);
    RUN_TEST(test_controls_api_get_torque_valid_wheel);

    // test controls_api_set_torque
    RUN_TEST(test_controls_api_set_torque_invalid_wheel);
    RUN_TEST(test_controls_api_set_torque_valid_wheel);

    // test controls_api_get_status
    RUN_TEST(test_controls_api_get_status_works_properly);

    // test controls_api_set_status
    RUN_TEST(test_controls_api_set_status_invalid_status);
    RUN_TEST(test_controls_api_set_status_valid_status);

    return UNITY_END();
}
