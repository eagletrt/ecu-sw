/**
 * \file test_fsm.c
 * \author Dorijan Di Zepp
 * \date 2026-05-28
 * \brief Unit tests for testing the fsm states and transition functions.
 * \details Given the amount of states, the objective is to verify that specific
 * states operate as expected and also the transition functions.
 */

#include <unity.h>
#include "ecu_fsm.h"

/* --- Test Cases --- */

/*!
 * \defgroup test_fsm_state_invariant Tests to verify that specific states
 * will modify the next state or that remains in the same state.
 * \{
 */

void test_fatal_state_stays_fatal(void) {
    state_t next_state = run_state(STATE_FATAL, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(STATE_FATAL, next_state, "The fatal state needs to stay fatal to avoid undesired behaviors");
}

void test_as_finished_stays_as_finished(void) {
    state_t next_state = run_state(STATE_AS_FINISHED, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(STATE_AS_FINISHED, next_state, "The AS should remain in finished unless specified differently");
}

void test_as_emergency_stays_as_emergency(void) {
    state_t next_state = run_state(STATE_AS_EMERGENCY, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(STATE_AS_EMERGENCY, next_state, "The AS should remain in emergency unless specified differently");
}

void test_as_r2d_is_a_transitory_state(void) {
    state_t next_state = run_state(STATE_AS_R2D, NULL);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(STATE_AS_R2D, next_state, "The AS R2D state needs to change the next state as it is a transitory state");
}
/*! \} */

/*!
 * \defgroup test_fsm_transition Tests to verify that certain transitions 
  * are taken or that they map into specific states.
 * \{
 */

void test_transition_from_idle_to_manual_wait_ts_precharge_exists(void) {
    TEST_ASSERT_NOT_NULL_MESSAGE(transition_table[STATE_IDLE][STATE_MANUAL_WAIT_TS_PRECHARGE], "A transition function is expected to move from idle to manual wait ts precharge");
}

void test_transition_from_idle_to_manual_wait_ts_precharge_is_start_ts_precharge(void) {
    transition_func_t *transition = transition_table[STATE_IDLE][STATE_MANUAL_WAIT_TS_PRECHARGE];
    TEST_ASSERT_EQUAL_MESSAGE(start_ts_precharge, transition, "The expected transition from idle to manual wait ts precharge is start ts precharge");
}

void test_transition_from_driving_to_manual_wait_ts_discharge_does_not_exist(void) {
    TEST_ASSERT_NULL_MESSAGE(transition_table[STATE_DRIVING][STATE_MANUAL_WAIT_TS_DISCHARGE], "It should not be possible to discharge the ts when exiting the driving state");
}

void test_transition_to_manual_wait_ts_discharge_for_error_or_normal_execution(void) {
    TEST_ASSERT_NOT_NULL_MESSAGE(transition_table[STATE_MANUAL_WAIT_TS_PRECHARGE][STATE_MANUAL_WAIT_TS_DISCHARGE], "If precharge fails, it should transition to its discharge");
    TEST_ASSERT_NOT_NULL_MESSAGE(transition_table[STATE_MANUAL_WAIT_INV_DISABLE][STATE_MANUAL_WAIT_TS_DISCHARGE], "After the inverters disabling, it should follow the ts discharge");
    TEST_ASSERT_NOT_NULL_MESSAGE(transition_table[STATE_WAIT_DRIVER][STATE_MANUAL_WAIT_TS_DISCHARGE], "If the driver is not ready, it should be possible to discharge the ts");
}

void test_transition_from_as_off_to_as_off_wait_ts_precharge(void) {
    TEST_ASSERT_NOT_NULL_MESSAGE(transition_table[STATE_AS_OFF][STATE_AS_OFF_WAIT_TS_PRECHARGE], "In as off it should be possible to start the precharge of the ts");
}

void test_no_loop_for_as_r2d(void) {
    TEST_ASSERT_NULL_MESSAGE(transition_table[STATE_AS_R2D][STATE_AS_R2D], "No transition function that allows a self loop to as r2d should be available");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(STATE_AS_R2D, do_as_r2d(NULL), "After executing as r2d, the next state has to be different from as r2d");
}

void test_as_emergency_triggers_inverters_disable(void) {
    TEST_ASSERT_EQUAL_MESSAGE(start_inv_disable, transition_table[STATE_AS_EMERGENCY][STATE_AS_EMERGENCY_WAIT_INV_DISABLE], "As emergency should call the inverters disabling");
}

/*! \} */

int main(void) {
    UNITY_BEGIN();

    /*!
     * \addtogroup test_fsm_state_invariant
     * \{
     */
    RUN_TEST(test_fatal_state_stays_fatal);
    RUN_TEST(test_as_finished_stays_as_finished);
    RUN_TEST(test_as_emergency_stays_as_emergency);
    RUN_TEST(test_as_r2d_is_a_transitory_state);
    /*! \} */

    /*!
     * \addtogroup test_fsm_transition
     * \{
     */
    RUN_TEST(test_transition_from_idle_to_manual_wait_ts_precharge_exists);
    RUN_TEST(test_transition_from_idle_to_manual_wait_ts_precharge_is_start_ts_precharge);
    RUN_TEST(test_transition_from_driving_to_manual_wait_ts_discharge_does_not_exist);
    RUN_TEST(test_transition_to_manual_wait_ts_discharge_for_error_or_normal_execution);
    RUN_TEST(test_transition_from_as_off_to_as_off_wait_ts_precharge);
    RUN_TEST(test_no_loop_for_as_r2d);
    RUN_TEST(test_as_emergency_triggers_inverters_disable);
    /*! \} */

    return UNITY_END();
}