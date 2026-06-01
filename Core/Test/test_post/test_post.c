/*!
 * \file test_post.c
 * \author Dorijan Di Zepp
 * \date 2026-06-01
 * \brief Unit tests using FFF for structural configuration verification of the POST module initialization.
 */

#include <unity.h>
#include <stdbool.h>
#include <string.h>
#include "post-api.h"
#include "fff.h"
#include "eagletrt-api.h"

DEFINE_FFF_GLOBALS;

// mocks for individual modules initialized by POST
FAKE_VALUE_FUNC(enum BuzzerReturnCode, mock_buzzer_on, uint32_t, float);
FAKE_VALUE_FUNC(enum BuzzerReturnCode, mock_buzzer_off);
FAKE_VALUE_FUNC(enum BuzzerReturnCode, mock_buzzer_delay, uint32_t, float, uint32_t);
FAKE_VALUE_FUNC(uint32_t, mock_buzzer_tick);
FAKE_VALUE_FUNC(enum InvertersReturnCode, mock_inverters_send_drive_command, enum InvertersDriveStatus, enum InvertersPosition);
FAKE_VALUE_FUNC(enum InvertersReturnCode, mock_inverters_set_torque, float, enum InvertersPosition);
FAKE_VALUE_FUNC(enum RaspberryReturnCode, mock_raspberry_pin_control, enum RaspberryControlPinState);
FAKE_VALUE_FUNC(enum TSReturnCode, mock_ts_send_command, enum TSCommand);

/*!
 * \brief Global configuration block instance managed per-test scenario execution frame.
 */
EAGLETRT_STATIC struct PostConfig post_config;

/*!
 * \brief Local utility function that populates a PostConfig instance with functional mock callbacks.
 *
 * This routine iterates through all configuration struct dependencies and maps function pointers
 * over to FFF's mock handlers.
 *
 * \param[out] cfg Pointer to the target configuration structure initialization target block.
 */
EAGLETRT_STATIC void build_default_valid_config(struct PostConfig *cfg) {
    // prepare buzzer callbacks arrays
    for (size_t i = 0; i < (size_t)BUZZER_TYPE_COUNT; i++) {
        cfg->buzzer_on_ptrs[i] = mock_buzzer_on;
        cfg->buzzer_off_ptrs[i] = mock_buzzer_off;
        cfg->buzzer_delay_ptrs[i] = mock_buzzer_delay;
        cfg->buzzer_tick_ptrs[i] = mock_buzzer_tick;
    }

    cfg->inverters_send_drive_command = mock_inverters_send_drive_command;
    cfg->inverters_set_torque = mock_inverters_set_torque;
    cfg->raspberry_pin_control = mock_raspberry_pin_control;
    cfg->ts_send_command = mock_ts_send_command;
}

void setUp(void) {
    // reset mocks state
    RESET_FAKE(mock_buzzer_on);
    RESET_FAKE(mock_buzzer_off);
    RESET_FAKE(mock_buzzer_delay);
    RESET_FAKE(mock_buzzer_tick);
    RESET_FAKE(mock_inverters_send_drive_command);
    RESET_FAKE(mock_inverters_set_torque);
    RESET_FAKE(mock_raspberry_pin_control);
    RESET_FAKE(mock_ts_send_command);

    FFF_RESET_HISTORY();

    // prepare post configuration structure
    memset(&post_config, 0, sizeof(struct PostConfig));
    build_default_valid_config(&post_config);
}

/* --- Test Cases --- */

/*!
 * \defgroup post_api_do_init Tests for post_api_do_init
 * \{
 */

void test_post_do_init_should_fail_if_configuration_pointer_is_null(void) {
    enum PostReturnCode rc = post_api_do_init(NULL);

    TEST_ASSERT_EQUAL_INT_MESSAGE(POST_RC_ERROR, rc, "POST initialization should immediately reject a NULL context configuration pointer.");
}

void test_post_do_init_should_return_ok_when_given_completely_valid_config(void) {
    enum PostReturnCode rc = post_api_do_init(&post_config);

    TEST_ASSERT_EQUAL_INT_MESSAGE(POST_RC_OK, rc, "POST initialization did not return POST_RC_OK when given a complete, verified callback configuration struct.");
}

void test_post_do_init_should_fail_if_a_buzzer_on_pointer_is_null(void) {
    // Zero out an explicit index to test boundary validation
    post_config.buzzer_on_ptrs[BUZZER_TYPE_COUNT / 2] = NULL;

    enum PostReturnCode rc = post_api_do_init(&post_config);

    TEST_ASSERT_EQUAL_INT_MESSAGE(POST_RC_ERROR, rc, "POST initialization should fail validation if any entry in the buzzer_on_ptrs array is NULL.");
}

void test_post_do_init_should_fail_if_a_buzzer_off_pointer_is_null(void) {
    post_config.buzzer_off_ptrs[BUZZER_TYPE_COUNT / 2] = NULL;

    enum PostReturnCode rc = post_api_do_init(&post_config);

    TEST_ASSERT_EQUAL_INT_MESSAGE(POST_RC_ERROR, rc, "POST initialization should fail validation if any entry in the buzzer_off_ptrs array is NULL.");
}

void test_post_do_init_should_fail_if_a_buzzer_delay_pointer_is_null(void) {
    post_config.buzzer_delay_ptrs[BUZZER_TYPE_COUNT / 2] = NULL;

    enum PostReturnCode rc = post_api_do_init(&post_config);

    TEST_ASSERT_EQUAL_INT_MESSAGE(POST_RC_ERROR, rc, "POST initialization should fail validation if any entry in the buzzer_delay_ptrs array is NULL.");
}

void test_post_do_init_should_fail_if_inverters_send_drive_command_callback_is_null(void) {
    post_config.inverters_send_drive_command = NULL;

    enum PostReturnCode rc = post_api_do_init(&post_config);

    TEST_ASSERT_EQUAL_INT_MESSAGE(POST_RC_ERROR, rc, "POST initialization should fail validation if the inverters send drive command callback is missing.");
}

void test_post_do_init_should_fail_if_inverters_set_torque_callback_is_null(void) {
    post_config.inverters_set_torque = NULL;

    enum PostReturnCode rc = post_api_do_init(&post_config);

    TEST_ASSERT_EQUAL_INT_MESSAGE(POST_RC_ERROR, rc, "POST initialization should fail validation if the inverters set torque callback is missing.");
}

void test_post_do_init_should_fail_if_raspberry_pin_control_callback_is_null(void) {
    post_config.raspberry_pin_control = NULL;

    enum PostReturnCode rc = post_api_do_init(&post_config);

    TEST_ASSERT_EQUAL_INT_MESSAGE(POST_RC_ERROR, rc, "POST initialization should fail validation if the Raspberry Pi pin control callback is missing.");
}

void test_post_do_init_should_fail_if_tractive_system_callback_is_null(void) {
    post_config.ts_send_command = NULL;

    enum PostReturnCode rc = post_api_do_init(&post_config);

    TEST_ASSERT_EQUAL_INT_MESSAGE(POST_RC_ERROR, rc, "POST initialization should fail validation if the tractive system send command callback is missing.");
}
/*! \} */

int main(void) {
    UNITY_BEGIN();

    /*!
     * \addtogroup post_api_do_init
     * \{
     */
    RUN_TEST(test_post_do_init_should_fail_if_configuration_pointer_is_null);
    RUN_TEST(test_post_do_init_should_return_ok_when_given_completely_valid_config);

    RUN_TEST(test_post_do_init_should_fail_if_a_buzzer_on_pointer_is_null);
    RUN_TEST(test_post_do_init_should_fail_if_a_buzzer_off_pointer_is_null);
    RUN_TEST(test_post_do_init_should_fail_if_a_buzzer_delay_pointer_is_null);
    RUN_TEST(test_post_do_init_should_fail_if_inverters_send_drive_command_callback_is_null);
    RUN_TEST(test_post_do_init_should_fail_if_inverters_set_torque_callback_is_null);
    RUN_TEST(test_post_do_init_should_fail_if_raspberry_pin_control_callback_is_null);
    RUN_TEST(test_post_do_init_should_fail_if_tractive_system_callback_is_null);
    /*! \} */

    return UNITY_END();
}