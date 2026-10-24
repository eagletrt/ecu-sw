/**
 * \file test_fsm.c
 * \author Dorijan Di Zepp
 * \author Thomas Moretti [majorfrost173@gmail.com]
 * \date 2026-05-28
 * \brief Unit tests for testing the fsm states and transition functions.
 * \details Given the amount of states, the objective is to verify that specific
 * states operate as expected and also the transition functions.
 */

#include <unity.h>
#include "eagletrt-fsm-api.h"
#include "eagletrt.h"
#include "fsm.h"

extern struct FSMHandler handler;
extern struct State states[STATE_COUNT];
extern void start_ts_precharge(void *);
extern void start_inv_disable(void *);
extern void routine_as_r2d(void *);

EAGLETRT_STATIC transition_function prv_get_transition_function(enum StateId state_from, enum StateId state_to) {
    if (state_from >= STATE_COUNT || state_to >= STATE_COUNT) {
        return nullptr;
    }

    for (uint8_t transitions = 0; transitions < states[state_from].num_transitions; ++transitions) {
        if ((states[state_from].transitions[transitions].to) == state_to) {
            return states[state_from].transitions[transitions].function;
        }
    }

    return nullptr;
}

void setUp(void) {
}

/* --- Test Cases --- */

/*!
 * \defgroup test_fsm_state_invariant Tests to verify that specific states
 * will modify the next state or that remains in the same state.
 * \{
 */

void test_fatal_state_stays_fatal(void) {
    fsm_api_init(&handler, states, STATE_COUNT, STATE_FATAL);
    fsm_api_transition_to(&handler, STATE_FATAL);
    fsm_api_run_state(&handler, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(STATE_FATAL, fsm_api_get_state(&handler), "The fatal state needs to stay fatal to avoid undesired behaviors");
}

void test_as_finished_stays_as_finished(void) {
    fsm_api_init(&handler, states, STATE_COUNT, STATE_AS_FINISHED);
    fsm_api_transition_to(&handler, STATE_AS_FINISHED);
    fsm_api_run_state(&handler, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(STATE_AS_FINISHED, fsm_api_get_state(&handler), "The AS should remain in finished unless specified differently");
}

void test_as_emergency_stays_as_emergency(void) {
    fsm_api_init(&handler, states, STATE_COUNT, STATE_AS_EMERGENCY);
    fsm_api_transition_to(&handler, STATE_AS_EMERGENCY);
    fsm_api_run_state(&handler, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(STATE_AS_EMERGENCY, fsm_api_get_state(&handler), "The AS should remain in emergency unless specified differently");
}

void test_as_r2d_is_a_transitory_state(void) {
    fsm_api_init(&handler, states, STATE_COUNT, STATE_AS_R2D);
    fsm_api_run_state(&handler, NULL);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(STATE_AS_R2D, fsm_api_get_state(&handler), "The AS R2D state needs to change the next state as it is a transitory state");
}
/*! \} */

/*!
 * \defgroup test_fsm_transition Tests to verify that certain transitions 
  * are taken or that they map into specific states.
 * \{
 */

void test_transition_from_idle_to_manual_wait_ts_precharge_exists(void) {
    transition_function transition_function = prv_get_transition_function(STATE_IDLE, STATE_MANUAL_WAIT_TS_PRECHARGE);
    TEST_ASSERT_NOT_NULL_MESSAGE(transition_function, "A transition function is expected to move from idle to manual wait ts precharge");
}

void test_transition_from_idle_to_manual_wait_ts_precharge_is_start_ts_precharge(void) {
    transition_function transition_function = prv_get_transition_function(STATE_IDLE, STATE_MANUAL_WAIT_TS_PRECHARGE);
    TEST_ASSERT_EQUAL_MESSAGE(start_ts_precharge, transition_function, "The expected transition from idle to manual wait ts precharge is start ts precharge");
}

void test_transition_from_driving_to_manual_wait_ts_discharge_does_not_exist(void) {
    transition_function transition_function = prv_get_transition_function(STATE_DRIVING, STATE_MANUAL_WAIT_TS_DISCHARGE);
    TEST_ASSERT_NULL_MESSAGE(transition_function, "It should not be possible to discharge the ts when exiting the driving state");
}

void test_transition_to_manual_wait_ts_discharge_for_error_or_normal_execution(void) {
    transition_function wait_precharge_to_wait_discharge = prv_get_transition_function(STATE_MANUAL_WAIT_TS_PRECHARGE, STATE_MANUAL_WAIT_TS_DISCHARGE);
    transition_function wait_inv_disable_to_wait_discharge = prv_get_transition_function(STATE_MANUAL_WAIT_INV_DISABLE, STATE_MANUAL_WAIT_TS_DISCHARGE);
    transition_function wait_driver_to_wait_discharge = prv_get_transition_function(STATE_WAIT_DRIVER, STATE_MANUAL_WAIT_TS_DISCHARGE);
    TEST_ASSERT_NOT_NULL_MESSAGE(wait_precharge_to_wait_discharge, "If precharge fails, it should transition to its discharge");
    TEST_ASSERT_NOT_NULL_MESSAGE(wait_inv_disable_to_wait_discharge, "After the inverters disabling, it should follow the ts discharge");
    TEST_ASSERT_NOT_NULL_MESSAGE(wait_driver_to_wait_discharge, "If the driver is not ready, it should be possible to discharge the ts");
}

void test_transition_from_as_off_to_as_off_wait_ts_precharge(void) {
    transition_function transition_function = prv_get_transition_function(STATE_AS_OFF, STATE_AS_OFF_WAIT_TS_PRECHARGE);
    TEST_ASSERT_NOT_NULL_MESSAGE(transition_function, "In as off it should be possible to start the precharge of the ts");
}

void test_no_loop_for_as_r2d(void) {
    transition_function transition_function = prv_get_transition_function(STATE_AS_R2D, STATE_AS_R2D);
    fsm_api_init(&handler, states, STATE_COUNT, STATE_AS_R2D);
    fsm_api_run_state(&handler, NULL);
    TEST_ASSERT_NULL_MESSAGE(transition_function, "No transition function that allows a self loop to as r2d should be available");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(STATE_AS_R2D, fsm_api_get_state(&handler), "After executing as r2d, the next state has to be different from as r2d");
}

void test_as_emergency_triggers_inverters_disable(void) {
    fsm_api_init(&handler, states, STATE_COUNT, STATE_INIT);
    transition_function transition_function = prv_get_transition_function(STATE_AS_EMERGENCY, STATE_AS_EMERGENCY_WAIT_INV_DISABLE);
    TEST_ASSERT_EQUAL_MESSAGE(start_inv_disable, transition_function, "As emergency should call the inverters disabling");
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
