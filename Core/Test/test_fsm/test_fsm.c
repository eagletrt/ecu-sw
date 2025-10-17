#include <unity.h>
#include "ecu_fsm.h"

void setUp(void) {
}

void tearDown(void) {
}

/**
 * test next state after running the current one
 */

void test_state_init_to_enable_inv_updates(void) {
    state_t next_state = run_state(STATE_INIT, NULL);
    TEST_ASSERT_EQUAL(STATE_ENABLE_INV_UPDATES, next_state);
}

void test_state_init_not_to_idle(void) {
    state_t next_state = run_state(STATE_INIT, NULL);
    TEST_ASSERT_NOT_EQUAL(STATE_IDLE, next_state);
}

void test_fatal_state_stays_fatal(void) {
    state_t next_state = run_state(STATE_FATAL, NULL);
    TEST_ASSERT_EQUAL(STATE_FATAL, next_state);
}

/**
 * test transition functions
 */

void test_transition_from_idle_to_wait_ts_precharge_exists(void) {
    TEST_ASSERT_NOT_NULL(transition_table[STATE_IDLE][STATE_WAIT_TS_PRECHARGE]);
}

void test_transition_from_idle_to_wait_ts_precharge_is_start_ts_precharge(void) {
    transition_func_t *transition = transition_table[STATE_IDLE][STATE_WAIT_TS_PRECHARGE];
    TEST_ASSERT_EQUAL(start_ts_precharge, transition);
}

void test_transition_from_disable_inv_drive_to_wait_ts_discharge_is_start_ts_discharge(void) {
    transition_func_t *transition = transition_table[STATE_DISABLE_INV_DRIVE][STATE_WAIT_TS_DISCHARGE];
    TEST_ASSERT_EQUAL(start_ts_discharge, transition);
}

void test_transition_from_rd2_to_wait_ts_discharge_does_not_exist(void) {
    TEST_ASSERT_NULL(transition_table[STATE_R2D][STATE_WAIT_TS_DISCHARGE]);
}

void test_transition_to_wait_ts_discharge_for_error_or_normal_execution(void) {
    TEST_ASSERT_NOT_NULL(transition_table[STATE_WAIT_TS_PRECHARGE][STATE_WAIT_TS_DISCHARGE]);
    TEST_ASSERT_NOT_NULL(transition_table[STATE_WAIT_DRIVER][STATE_WAIT_TS_DISCHARGE]);
    TEST_ASSERT_NOT_NULL(transition_table[STATE_ENABLE_AS_DRIVER][STATE_WAIT_TS_DISCHARGE]);
    TEST_ASSERT_NOT_NULL(transition_table[STATE_DISABLE_INV_DRIVE][STATE_WAIT_TS_DISCHARGE]);
}

/**
 * test possible next states after the execution of a current state
 */

void test_do_init_returns_valid_state(void) {
    state_t result = do_init(NULL);
    TEST_ASSERT_TRUE(result == STATE_ENABLE_INV_UPDATES || result == STATE_FATAL);
}

void test_do_flash_returns_valid_state(void) {
    state_t result = do_flash(NULL);
    TEST_ASSERT_TRUE(result == NO_CHANGE || result == STATE_IDLE || result == STATE_FLASH);
}

void test_do_pause_returns_valid_state(void) {
    state_t result = do_pause(NULL);
    TEST_ASSERT_TRUE(result == NO_CHANGE || result == STATE_IDLE || result == STATE_PAUSE);
}

void test_do_check_inv_settings_valid_tate(void) {
    state_t result = do_check_inv_settings(NULL);
    TEST_ASSERT_TRUE(result == NO_CHANGE || result == STATE_FATAL || result == STATE_IDLE);
}

void test_do_wait_driver_returns_valid_state(void) {
    state_t result = do_wait_driver(NULL);
    TEST_ASSERT_TRUE(result == NO_CHANGE || result == STATE_WAIT_DRIVER || result == STATE_ENABLE_AS_DRIVER || result == STATE_ENABLE_INV_DRIVE);
}

void test_do_r2d_returns_valid_state(void) {
    state_t result = do_r2d(NULL);
    TEST_ASSERT_TRUE(result == NO_CHANGE || result == STATE_R2D || result == STATE_DISABLE_INV_DRIVE || result == STATE_DISABLE_AS_DRIVER);
}

void test_do_wait_ts_discharge_valid_state(void) {
    state_t result = do_wait_ts_discharge(NULL);
    TEST_ASSERT_TRUE(result == NO_CHANGE || result == STATE_WAIT_TS_DISCHARGE || result == STATE_IDLE);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_state_init_to_enable_inv_updates);
    RUN_TEST(test_state_init_not_to_idle);
    RUN_TEST(test_fatal_state_stays_fatal);

    RUN_TEST(test_transition_from_idle_to_wait_ts_precharge_exists);
    RUN_TEST(test_transition_from_idle_to_wait_ts_precharge_is_start_ts_precharge);
    RUN_TEST(test_transition_from_disable_inv_drive_to_wait_ts_discharge_is_start_ts_discharge);
    RUN_TEST(test_transition_from_rd2_to_wait_ts_discharge_does_not_exist);
    RUN_TEST(test_transition_to_wait_ts_discharge_for_error_or_normal_execution);

    RUN_TEST(test_do_init_returns_valid_state);
    RUN_TEST(test_do_flash_returns_valid_state);
    RUN_TEST(test_do_flash_returns_valid_state);
    RUN_TEST(test_do_check_inv_settings_valid_tate);
    RUN_TEST(test_do_wait_driver_returns_valid_state);
    RUN_TEST(test_do_r2d_returns_valid_state);
    RUN_TEST(test_do_wait_ts_discharge_valid_state);

    return UNITY_END();
}