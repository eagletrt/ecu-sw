/**
 * \file test_post.c
 * \author Dorijan Di Zepp
 * \date 2026-04-13
 * \brief Unit tests using FFF for testing the POST module
 */

#include <unity.h>
#include <stdbool.h>
#include "post-api.h"
#include "fff.h"
#include "eagletrt-api.h"

DEFINE_FFF_GLOBALS;

// set of possible real callbacks to be used during POST
FAKE_VALUE_FUNC(enum PostReturnCode, check_can_bus);
FAKE_VALUE_FUNC(enum PostReturnCode, check_pedals_availability);
FAKE_VALUE_FUNC(enum PostReturnCode, check_buzzers);

void setUp(void) {
    FFF_RESET_HISTORY();
    RESET_FAKE(check_can_bus);
    RESET_FAKE(check_pedals_availability);
    RESET_FAKE(check_buzzers);

    // assume by default that all callbacks pass
    check_can_bus_fake.return_val = POST_RC_OK;
    check_pedals_availability_fake.return_val = POST_RC_OK;
    check_buzzers_fake.return_val = POST_RC_OK;
}

/* --- Test Cases --- */

/*!
 * \defgroup post_api_execute Tests for post_api_execute function
 * \{
 */

void test_post_execute_null_configuration(void) {
    TEST_ASSERT_EQUAL_MESSAGE(POST_RC_ERROR, post_api_execute(NULL), "POST should fail if configuration is NULL");
}

void test_post_execute_null_test_table(void) {
    struct PostConfig cfg = { .test_table = NULL, .num_tests = 0 };

    TEST_ASSERT_EQUAL_MESSAGE(POST_RC_ERROR, post_api_execute(&cfg), "POST should fail if test table is NULL");
}

void test_post_execute_null_test_name(void) {
    const struct PostTask test_table[] = {
        { NULL, check_can_bus },
        { "PEDALS_HW", check_pedals_availability },
        { NULL, check_buzzers },
    };

    struct PostConfig cfg = { .test_table = test_table,
                              .num_tests = sizeof(test_table) / sizeof(struct PostTask) };

    TEST_ASSERT_EQUAL_MESSAGE(POST_RC_OK, post_api_execute(&cfg), "POST should pass even with no test name defined");

    // verify every registered function was actually executed
    TEST_ASSERT_EQUAL_MESSAGE(1, check_can_bus_fake.call_count, "Can bus should be called once");
    TEST_ASSERT_EQUAL_MESSAGE(1, check_pedals_availability_fake.call_count, "Pedals availability should be called once");
    TEST_ASSERT_EQUAL_MESSAGE(1, check_buzzers_fake.call_count, "Buzzers should be called once");
}

void test_post_execute_successful_sequence(void) {
    const struct PostTask test_table[] = {
        { "CAN_NODES", check_can_bus },
        { "PEDALS_HW", check_pedals_availability },
        { "BUZZER_API", check_buzzers },
    };

    struct PostConfig cfg = { .test_table = test_table,
                              .num_tests = sizeof(test_table) / sizeof(struct PostTask) };

    TEST_ASSERT_EQUAL_MESSAGE(POST_RC_OK, post_api_execute(&cfg), "POST should pass when all hardware checks return OK");

    // verify every registered function was actually executed
    TEST_ASSERT_EQUAL_MESSAGE(1, check_can_bus_fake.call_count, "Can bus should be called once");
    TEST_ASSERT_EQUAL_MESSAGE(1, check_pedals_availability_fake.call_count, "Pedals availability should be called once");
    TEST_ASSERT_EQUAL_MESSAGE(1, check_buzzers_fake.call_count, "Buzzers should be called once");
}

void test_post_execute_fails_when_callback_fails(void) {
    // simulate a hardware failure in the pedals
    check_pedals_availability_fake.return_val = POST_RC_ERROR;

    const struct PostTask test_table[] = {
        { "CAN_NODES", check_can_bus },
        { "PEDALS_HW", check_pedals_availability },
        { "BUZZER_API", check_buzzers },
    };

    struct PostConfig cfg = { .test_table = test_table,
                              .num_tests = sizeof(test_table) / sizeof(struct PostTask) };

    TEST_ASSERT_EQUAL_MESSAGE(POST_RC_ERROR, post_api_execute(&cfg), "POST should return ERROR if any hardware check fails");

    // because we return early on failures, not all callbacks might be called
    TEST_ASSERT_EQUAL_MESSAGE(1, check_can_bus_fake.call_count, "Can bus should be called once");
    TEST_ASSERT_EQUAL_MESSAGE(1, check_pedals_availability_fake.call_count, "Pedals availability should be called in order to fail");
    TEST_ASSERT_EQUAL_MESSAGE(0, check_buzzers_fake.call_count, "Subsequent tests should not run after a failure");
}

void test_post_execute_skips_remaining_tests_when_count_is_low(void) {
    const struct PostTask test_table[] = {
        { "CAN_BUS", check_can_bus },
        { "PEDALS", check_pedals_availability },
        { "BUZZER", check_buzzers }
    };

    // "under-count" by saying there is one less test
    struct PostConfig cfg = {
        .test_table = test_table,
        .num_tests = (sizeof(test_table) / sizeof(struct PostTask)) - 1
    };

    TEST_ASSERT_EQUAL_MESSAGE(POST_RC_OK, post_api_execute(&cfg), "POST should still return OK if it finishes the requested number of tests");

    // verify only the first two were called
    TEST_ASSERT_EQUAL_MESSAGE(1, check_can_bus_fake.call_count, "Can bus should be called once");
    TEST_ASSERT_EQUAL_MESSAGE(1, check_pedals_availability_fake.call_count, "Pedals availability should be called once");

    // verify the third one was skipped
    TEST_ASSERT_EQUAL_MESSAGE(0, check_buzzers_fake.call_count, "The third test should not have been executed because num_tests was 2");
}

/*! \} */

int main(void) {
    UNITY_BEGIN();

    /*!
     * \addtogroup post_api_execute
     * \{
     */
    RUN_TEST(test_post_execute_null_configuration);
    RUN_TEST(test_post_execute_null_test_table);
    RUN_TEST(test_post_execute_null_test_name);
    RUN_TEST(test_post_execute_successful_sequence);
    RUN_TEST(test_post_execute_fails_when_callback_fails);
    RUN_TEST(test_post_execute_skips_remaining_tests_when_count_is_low);
    /*! \} */

    return UNITY_END();
}