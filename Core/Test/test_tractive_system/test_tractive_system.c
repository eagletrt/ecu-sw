#include <unity.h>
#include "tractive_system.h"

/* Mocking State */
static int mock_on_cmd_count = 0;
static int mock_off_cmd_count = 0;
static enum TSReturnCode next_mock_return = TS_RC_OK;

/* Mock implementations */
enum TSReturnCode mock_send_command(enum TSCommand cmd) {
    if (cmd == TS_COMMAND_ON)
        mock_on_cmd_count++;
    if (cmd == TS_COMMAND_OFF)
        mock_off_cmd_count++;
    return next_mock_return;
}

// ------------------------------------------

void setUp(void) {
    mock_on_cmd_count = 0;
    mock_off_cmd_count = 0;
    next_mock_return = TS_RC_OK;
    TS_init(mock_send_command);
}

void tearDown(void) {
}

/* --- Tests --- */

/*!
 * \brief Verifies that the status starts at UNKNOWN and requests are cleared.
 */
void test_ts_init_state(void) {
    TEST_ASSERT_EQUAL_MESSAGE(TS_STATUS_UNKNOWN, TS_get_status(), "TS should start in UNKNOWN state");
    TEST_ASSERT_FALSE_MESSAGE(TS_is_power_on_requested(), "No power on should be requested initially");
    TEST_ASSERT_FALSE_MESSAGE(TS_is_power_off_requested(), "No power off should be requested initially");
}

/*!
 * \brief Verifies that request_on is set only if hardware command succeeds and 
 * status is appropriate.
 */
void test_ts_request_on_flow(void) {
    TS_set_status(TS_STATUS_OFF);

    // successful request
    enum TSReturnCode rc = TS_request_power_on();
    TEST_ASSERT_EQUAL_MESSAGE(TS_RC_OK, rc, "Request ON should return OK when system is OFF");
    TEST_ASSERT_TRUE_MESSAGE(TS_is_power_on_requested(), "Power ON request flag should be set");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, mock_on_cmd_count, "Hardware ON command should be sent exactly once");

    // test hardware failure
    TS_clear_request();
    next_mock_return = TS_RC_ERROR;
    rc = TS_request_power_on();
    TEST_ASSERT_EQUAL_MESSAGE(TS_RC_ERROR, rc, "Request ON should return error if hardware callback fails");
    TEST_ASSERT_FALSE_MESSAGE(TS_is_power_on_requested(), "Power ON request should not be set if hardware command fails");
}

/**ì!
 * \brief Verifies that we cannot request power on if the system is already ON.
 */
void test_ts_request_on_guard(void) {
    TS_set_status(TS_STATUS_ON);

    // Requesting ON while already ON should do nothing (return OK but no HW call)
    enum TSReturnCode rc = TS_request_power_on();
    TEST_ASSERT_EQUAL_MESSAGE(TS_RC_OK, rc, "Request ON should return OK even if redundant");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, mock_on_cmd_count, "No hardware command should be sent if system is already ON");
    TEST_ASSERT_FALSE_MESSAGE(TS_is_power_on_requested(), "Power ON request flag should not be set if already ON");
}

/*!
 * \brief Verifies that OFF can be requested from any state, including FATAL.
 */
void test_ts_request_off_from_anywhere(void) {
    TS_set_status(TS_STATUS_FATAL);

    enum TSReturnCode rc = TS_request_power_off();
    TEST_ASSERT_EQUAL_MESSAGE(TS_RC_OK, rc, "Request OFF should be allowed from FATAL state");
    TEST_ASSERT_TRUE_MESSAGE(TS_is_power_off_requested(), "Power OFF request flag should be set");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, mock_off_cmd_count, "Hardware OFF command should be triggered");
}

/*!
 * \brief Verifies the string representation for logging.
 */
void test_ts_get_state_name(void) {
    TEST_ASSERT_EQUAL_STRING_MESSAGE("OFF", TS_get_state_name(TS_STATUS_OFF), "Incorrect string for OFF state");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("FATAL", TS_get_state_name(TS_STATUS_FATAL), "Incorrect string for FATAL state");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("UNKNOWN", TS_get_state_name(99), "Invalid state index should return default 'UNKNOWN'");
}

/*!
 * \brief Verifies that clearing requests works independently of state.
 */
void test_ts_clear_requests(void) {
    TS_set_status(TS_STATUS_OFF);
    TS_request_power_on();
    TEST_ASSERT_TRUE_MESSAGE(TS_is_power_on_requested(), "Request ON should be set before clear");

    TS_clear_request();
    TEST_ASSERT_FALSE_MESSAGE(TS_is_power_on_requested(), "Power ON request should be cleared after call");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ts_init_state);
    RUN_TEST(test_ts_request_on_flow);
    RUN_TEST(test_ts_request_on_guard);
    RUN_TEST(test_ts_request_off_from_anywhere);
    RUN_TEST(test_ts_get_state_name);
    RUN_TEST(test_ts_clear_requests);
    return UNITY_END();
}