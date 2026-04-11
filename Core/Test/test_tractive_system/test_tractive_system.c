/**
 * \file test_tractive_system.c
 * \author Dorijan Di Zepp
 * \date 2026-03-28
 * \brief Unit tests using FFF for testing the TS module
 */

#include <unity.h>
#include "tractive-system-api.h"
#include "fff.h"
#include "eagletrt-api.h"

DEFINE_FFF_GLOBALS;

// mock for TS module
FAKE_VALUE_FUNC(enum TSReturnCode, send_ts_command, enum TSCommand);

void setUp(void) {
    ts_api_init(send_ts_command);

    // reset mock state
    RESET_FAKE(send_ts_command);

    FFF_RESET_HISTORY();
}

void tearDown(void) {
}

/* --- Tests --- */

/*!
 * \defgroup ts_api_init Tests for ts_api_init function
 * \{
 */

void test_ts_init_null_callback(void) {
    TEST_ASSERT_EQUAL_MESSAGE(TS_RC_ERROR, ts_api_init(NULL), "Initialization should fail when no callback is passed");
}

void test_ts_init_correct_initialization(void) {
    TEST_ASSERT_EQUAL_MESSAGE(TS_RC_OK, ts_api_init(send_ts_command), "Initialization should pass when a correct callback is passed");
}

/*! \} */

/*!
 * \defgroup ts_api_request_command Tests for ts_api_request_command function
 * \{
 */

void test_ts_request_command_successful_call(void) {
    // test success call
    send_ts_command_fake.return_val = TS_RC_OK;
    TEST_ASSERT_EQUAL_MESSAGE(TS_RC_OK, ts_api_request_command(TS_COMMAND_ON), "A correct initialization should allow a correct sending of a command");
    TEST_ASSERT_EQUAL_MESSAGE(TS_COMMAND_ON, send_ts_command_fake.arg0_val, "The command passed should be COMMAND ON");
    TEST_ASSERT_EQUAL_MESSAGE(1, send_ts_command_fake.call_count, "The callback should have been called exactly once");
}

void test_ts_request_command_failed_call(void) {
    // test failed call
    send_ts_command_fake.return_val = TS_RC_ERROR;
    TEST_ASSERT_EQUAL_MESSAGE(TS_RC_ERROR, ts_api_request_command(TS_COMMAND_OFF), "In case the callback fails, the return code should be forwarded to the caller");
    TEST_ASSERT_EQUAL_MESSAGE(TS_COMMAND_OFF, send_ts_command_fake.arg0_val, "The command passed should be COMMAND OFF");
    TEST_ASSERT_EQUAL_MESSAGE(1, send_ts_command_fake.call_count, "The callback should have been called exactly once");
}

/*! \} */

int main(void) {
    UNITY_BEGIN();

    /*!
     * \addtogroup ts_api_init
     * \{
     */
    RUN_TEST(test_ts_init_null_callback);
    RUN_TEST(test_ts_init_correct_initialization);
    /*! \} */

    /*!
     * \addtogroup ts_api_request_command
     * \{
     */
    RUN_TEST(test_ts_request_command_successful_call);
    RUN_TEST(test_ts_request_command_failed_call);
    /*! \} */

    return UNITY_END();
}