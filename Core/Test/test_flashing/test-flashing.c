#include "unity.h"
#include "fff.h"
#include "flashing-api.h"
#include "watchdogs-api.h"
#include "eagletrt-api.h"
#include <string.h>

DEFINE_FFF_GLOBALS;

extern struct FlashingHandler flashing_handler;
extern void prv_flashing_on_timeout(void);

EAGLETRT_STATIC struct WatchdogHandler watchdog_handler;
EAGLETRT_STATIC uint32_t test_tick = 0U;

FAKE_VOID_FUNC(fake_reset);

/* Tick source required by the watchdog module */
EAGLETRT_STATIC uint32_t prv_test_get_tick(void) {
    return test_tick;
}

void setUp(void) {
    RESET_FAKE(fake_reset);
    FFF_RESET_HISTORY();

    test_tick = 0U;

    watchdogs_api_init_pool(&watchdog_handler, prv_test_get_tick);
    flashing_api_init(&watchdog_handler, fake_reset);
}

/* --- Test Cases --- */

/*!
 * \defgroup flashing_api_init Tests for flashing_api_init
 * \{
 */

void test_flashing_api_init_should_fail_on_null_watchdog_handler(void) {
    memset(&flashing_handler, 0, sizeof(flashing_handler));

    enum FlashingReturnCode rc = flashing_api_init(NULL, fake_reset);

    TEST_ASSERT_EQUAL_MESSAGE(FLASHING_RC_NULL_POINTER, rc, "Module initialization did not fail when passed a NULL watchdog handler");
}

void test_flashing_api_init_should_fail_on_null_system_reset_callback(void) {
    memset(&flashing_handler, 0, sizeof(flashing_handler));

    enum FlashingReturnCode rc = flashing_api_init(&watchdog_handler, NULL);

    TEST_ASSERT_EQUAL_MESSAGE(FLASHING_RC_NULL_POINTER, rc, "Module initialization did not fail when passed a NULL system reset callback");
}

void test_flashing_api_init_should_not_modify_handler_on_invalid_parameters(void) {
    memset(&flashing_handler, 0, sizeof(flashing_handler));

    flashing_api_init(NULL, fake_reset);
    flashing_api_init(&watchdog_handler, NULL);

    TEST_ASSERT_NULL_MESSAGE(flashing_handler.watchdog_handler, "Module stored a watchdog handler even though initialization parameters were invalid");
    TEST_ASSERT_NULL_MESSAGE(flashing_handler.system_reset, "Module stored a reset callback even though initialization parameters were invalid");
}

void test_flashing_api_init_should_succeed_with_correct_parameters(void) {
    memset(&flashing_handler, 0, sizeof(flashing_handler));

    enum FlashingReturnCode rc = flashing_api_init(&watchdog_handler, fake_reset);

    TEST_ASSERT_EQUAL_MESSAGE(FLASHING_RC_OK, rc, "Module initialization reported an error under nominal, valid parameters");
}

void test_flashing_api_init_should_clear_previous_state(void) {
    flashing_handler.is_flashing = true;
    flashing_handler.is_timed_out = true;

    flashing_api_init(&watchdog_handler, fake_reset);

    TEST_ASSERT_FALSE_MESSAGE(flashing_handler.is_flashing, "Module initialization did not clear a stale flashing flag");
    TEST_ASSERT_FALSE_MESSAGE(flashing_handler.is_timed_out, "Module initialization did not clear a stale timed out flag");
}
/*! \} */

/*!
 * \defgroup flashing_api_perform_reset Tests for flashing_api_perform_reset
 * \{
 */

void test_flashing_api_perform_reset_should_fail_when_not_initialized(void) {
    memset(&flashing_handler, 0, sizeof(flashing_handler));

    enum FlashingReturnCode rc = flashing_api_perform_reset();

    TEST_ASSERT_EQUAL_MESSAGE(FLASHING_RC_NULL_POINTER, rc, "Module attempted a system reset without a registered reset callback");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, fake_reset_fake.call_count, "Module invoked a reset callback that was never registered");
}

void test_flashing_api_perform_reset_should_succeed(void) {
    enum FlashingReturnCode rc = flashing_api_perform_reset();

    TEST_ASSERT_EQUAL_MESSAGE(FLASHING_RC_OK, rc, "The reset routine reported an error under nominal conditions");
}

void test_flashing_api_perform_reset_should_invoke_system_reset_callback(void) {
    flashing_api_perform_reset();

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, fake_reset_fake.call_count, "The module failed to invoke the registered system reset callback exactly once");
}
/*! \} */

/*!
 * \defgroup flashing_api_restart_watchdog Tests for flashing_api_restart_watchdog
 * \{
 */

void test_flashing_api_restart_watchdog_should_succeed(void) {
    enum FlashingReturnCode rc = flashing_api_restart_watchdog();

    TEST_ASSERT_EQUAL_MESSAGE(FLASHING_RC_OK, rc, "Restarting the flashing watchdog reported an error under nominal conditions");
}

void test_flashing_api_restart_watchdog_should_clear_timed_out_flag(void) {
    flashing_handler.is_timed_out = true;

    flashing_api_restart_watchdog();

    TEST_ASSERT_FALSE_MESSAGE(flashing_api_is_timed_out(), "Restarting the watchdog did not clear the timed out flag");
}

void test_flashing_api_restart_watchdog_should_not_affect_flashing_flag(void) {
    flashing_handler.is_flashing = true;

    flashing_api_restart_watchdog();

    TEST_ASSERT_TRUE_MESSAGE(flashing_api_is_flashing(), "Restarting the watchdog unexpectedly modified the flashing flag");
}
/*! \} */

/*!
 * \defgroup flashing_api_pet_watchdog Tests for flashing_api_pet_watchdog
 * \{
 */

void test_flashing_api_pet_watchdog_should_fail_when_watchdog_not_running(void) {
    enum FlashingReturnCode rc = flashing_api_pet_watchdog();

    TEST_ASSERT_EQUAL_MESSAGE(FLASHING_RC_ERROR, rc, "Petting a watchdog that was never started did not report an error");
}

void test_flashing_api_pet_watchdog_should_succeed(void) {
    flashing_api_restart_watchdog();

    enum FlashingReturnCode rc = flashing_api_pet_watchdog();

    TEST_ASSERT_EQUAL_MESSAGE(FLASHING_RC_OK, rc, "Petting the flashing watchdog reported an error under nominal conditions");
}

void test_flashing_api_pet_watchdog_should_not_clear_timed_out_flag(void) {
    flashing_api_restart_watchdog();
    flashing_handler.is_timed_out = true;

    flashing_api_pet_watchdog();

    TEST_ASSERT_TRUE_MESSAGE(flashing_api_is_timed_out(), "Petting the watchdog unexpectedly cleared the timed out flag");
}
/*! \} */

/*!
 * \defgroup flashing_api_is_timed_out Tests for flashing_api_is_timed_out
 * \{
 */

void test_flashing_api_is_timed_out_should_be_false_after_init(void) {
    TEST_ASSERT_FALSE_MESSAGE(flashing_api_is_timed_out(), "The timed out flag is set right after initialization");
}

void test_flashing_api_is_timed_out_should_be_true_after_timeout(void) {
    prv_flashing_on_timeout();

    TEST_ASSERT_TRUE_MESSAGE(flashing_api_is_timed_out(), "The timeout callback did not set the timed out flag");
}

void test_flashing_api_is_timed_out_should_be_false_after_timeout_and_restart(void) {
    prv_flashing_on_timeout();

    flashing_api_restart_watchdog();

    TEST_ASSERT_FALSE_MESSAGE(flashing_api_is_timed_out(), "The timed out flag stayed set after the watchdog was restarted");
}
/*! \} */

/*!
 * \defgroup flashing_api_is_flashing Tests for flashing_api_is_flashing
 * \{
 */

void test_flashing_api_is_flashing_should_be_false_after_init(void) {
    TEST_ASSERT_FALSE_MESSAGE(flashing_api_is_flashing(), "The flashing flag is set right after initialization");
}

void test_flashing_api_is_flashing_should_not_be_affected_by_timeout(void) {
    flashing_handler.is_flashing = true;

    prv_flashing_on_timeout();

    TEST_ASSERT_TRUE_MESSAGE(flashing_api_is_flashing(), "A timeout unexpectedly modified the flashing flag");
}
/*! \} */

/*!
 * \defgroup flashing_api_set_flashing Tests for flashing_api_set_flashing
 * \{
 */

void test_flashing_api_set_flashing_should_succeed(void) {
    enum FlashingReturnCode rc = flashing_api_set_flashing(true);

    TEST_ASSERT_EQUAL_MESSAGE(FLASHING_RC_OK, rc, "Setting the flashing flag reported an error under nominal conditions");
}

void test_flashing_api_set_flashing_should_set_flag(void) {
    flashing_api_set_flashing(true);

    TEST_ASSERT_TRUE_MESSAGE(flashing_api_is_flashing(), "The flashing flag was not set after requesting it");
}

void test_flashing_api_set_flashing_should_clear_flag(void) {
    flashing_api_set_flashing(true);

    flashing_api_set_flashing(false);

    TEST_ASSERT_FALSE_MESSAGE(flashing_api_is_flashing(), "The flashing flag was not cleared after requesting it");
}

void test_flashing_api_set_flashing_should_not_affect_timed_out_flag(void) {
    flashing_handler.is_timed_out = true;

    flashing_api_set_flashing(true);

    TEST_ASSERT_TRUE_MESSAGE(flashing_api_is_timed_out(), "Setting the flashing flag unexpectedly modified the timed out flag");
}
/*! \} */

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_flashing_api_init_should_fail_on_null_watchdog_handler);
    RUN_TEST(test_flashing_api_init_should_fail_on_null_system_reset_callback);
    RUN_TEST(test_flashing_api_init_should_not_modify_handler_on_invalid_parameters);
    RUN_TEST(test_flashing_api_init_should_succeed_with_correct_parameters);
    RUN_TEST(test_flashing_api_init_should_clear_previous_state);

    RUN_TEST(test_flashing_api_perform_reset_should_fail_when_not_initialized);
    RUN_TEST(test_flashing_api_perform_reset_should_succeed);
    RUN_TEST(test_flashing_api_perform_reset_should_invoke_system_reset_callback);

    RUN_TEST(test_flashing_api_restart_watchdog_should_succeed);
    RUN_TEST(test_flashing_api_restart_watchdog_should_clear_timed_out_flag);
    RUN_TEST(test_flashing_api_restart_watchdog_should_not_affect_flashing_flag);

    RUN_TEST(test_flashing_api_pet_watchdog_should_fail_when_watchdog_not_running);
    RUN_TEST(test_flashing_api_pet_watchdog_should_succeed);
    RUN_TEST(test_flashing_api_pet_watchdog_should_not_clear_timed_out_flag);

    RUN_TEST(test_flashing_api_is_timed_out_should_be_false_after_init);
    RUN_TEST(test_flashing_api_is_timed_out_should_be_true_after_timeout);
    RUN_TEST(test_flashing_api_is_timed_out_should_be_false_after_timeout_and_restart);

    RUN_TEST(test_flashing_api_is_flashing_should_be_false_after_init);
    RUN_TEST(test_flashing_api_is_flashing_should_not_be_affected_by_timeout);

    RUN_TEST(test_flashing_api_set_flashing_should_succeed);
    RUN_TEST(test_flashing_api_set_flashing_should_set_flag);
    RUN_TEST(test_flashing_api_set_flashing_should_clear_flag);
    RUN_TEST(test_flashing_api_set_flashing_should_not_affect_timed_out_flag);

    return UNITY_END();
}
