/**
 * \file test_buzzer.c
 * \author Dorijan Di Zepp
 * \date 2026-04-08
 * \brief Unit tests using FFF for testing the buzzer module
 * \note Exhaustive testing of every buzzer instance (e.g R2D vs. ASSI) is 
 * unnecessary for most logic tests (e.g getters and setters), as the API 
 * uses the buzzer type as a direct index into a common handler array; 
 * testing one instance validates the shared logic.
 */

#include <unity.h>
#include <stdbool.h>
#include "buzzer-api.h"
#include "fff.h"
#include "eagletrt-api.h"

extern struct BuzzerHandler buzzer_handlers[];

DEFINE_FFF_GLOBALS;

// mocks for R2D buzzer
FAKE_VALUE_FUNC(enum BuzzerReturnCode, buzzer_on_r2d, uint32_t, float);
FAKE_VALUE_FUNC(enum BuzzerReturnCode, buzzer_off_r2d);
FAKE_VALUE_FUNC(enum BuzzerReturnCode, buzzer_sync_r2d, uint32_t, float, uint32_t);
FAKE_VALUE_FUNC(uint32_t, get_tick_r2d);

// mocks for ASSI buzzer
FAKE_VALUE_FUNC(enum BuzzerReturnCode, buzzer_on_assi, uint32_t, float);
FAKE_VALUE_FUNC(enum BuzzerReturnCode, buzzer_off_assi);
FAKE_VALUE_FUNC(enum BuzzerReturnCode, buzzer_sync_assi, uint32_t, float, uint32_t);
FAKE_VALUE_FUNC(uint32_t, get_tick_assi);

void setUp(void) {
    // prepare the callbacks array for bulk initialization
    // the order should correspond to the BuzzerType enum indices
    buzzer_on_callback ons[BUZZER_TYPE_COUNT] = { buzzer_on_r2d, buzzer_on_assi };
    buzzer_off_callback offs[BUZZER_TYPE_COUNT] = { buzzer_off_r2d, buzzer_off_assi };
    buzzer_delay_callback syncs[BUZZER_TYPE_COUNT] = { buzzer_sync_r2d, buzzer_sync_assi };
    buzzer_tick_callback ticks[BUZZER_TYPE_COUNT] = { get_tick_r2d, get_tick_assi };

    // initialize the API once for all buzzers
    buzzer_api_init(ons, offs, syncs, ticks);

    // reset mocks state
    RESET_FAKE(buzzer_on_r2d);
    RESET_FAKE(buzzer_off_r2d);
    RESET_FAKE(buzzer_sync_r2d);
    RESET_FAKE(get_tick_r2d);

    RESET_FAKE(buzzer_on_assi);
    RESET_FAKE(buzzer_off_assi);
    RESET_FAKE(buzzer_sync_assi);
    RESET_FAKE(get_tick_assi);

    FFF_RESET_HISTORY();
}

void tearDown(void) {
    buzzer_api_reset(BUZZER_TYPE_R2D);
    buzzer_api_reset(BUZZER_TYPE_ASSI);
}

/* --- Test Cases --- */

/*!
 * \defgroup buzzer_api_init Tests for buzzer_api_init function
 * \{
 */

void test_buzzer_api_init_null_on_array(void) {
    buzzer_off_callback offs[BUZZER_TYPE_COUNT] = { buzzer_off_r2d, buzzer_off_assi };
    buzzer_delay_callback syncs[BUZZER_TYPE_COUNT] = { buzzer_sync_r2d, buzzer_sync_assi };
    buzzer_tick_callback ticks[BUZZER_TYPE_COUNT] = { get_tick_r2d, get_tick_assi };

    enum BuzzerReturnCode rc = buzzer_api_init(NULL, offs, syncs, ticks);

    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, rc, "Init should fail if on_ptrs array is NULL");
    TEST_ASSERT_EQUAL(0, buzzer_off_r2d_fake.call_count);
}

void test_buzzer_api_init_null_off_array(void) {
    buzzer_on_callback ons[BUZZER_TYPE_COUNT] = { buzzer_on_r2d, buzzer_on_assi };
    buzzer_delay_callback syncs[BUZZER_TYPE_COUNT] = { buzzer_sync_r2d, buzzer_sync_assi };
    buzzer_tick_callback ticks[BUZZER_TYPE_COUNT] = { get_tick_r2d, get_tick_assi };

    enum BuzzerReturnCode rc = buzzer_api_init(ons, NULL, syncs, ticks);

    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, rc, "Init should fail if off_ptrs array is NULL");
    TEST_ASSERT_EQUAL(0, buzzer_off_r2d_fake.call_count);
}

void test_buzzer_api_init_null_sync_array(void) {
    buzzer_on_callback ons[BUZZER_TYPE_COUNT] = { buzzer_on_r2d, buzzer_on_assi };
    buzzer_off_callback offs[BUZZER_TYPE_COUNT] = { buzzer_off_r2d, buzzer_off_assi };
    buzzer_tick_callback ticks[BUZZER_TYPE_COUNT] = { get_tick_r2d, get_tick_assi };

    enum BuzzerReturnCode rc = buzzer_api_init(ons, offs, NULL, ticks);

    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, rc, "Init should fail if play_sync_ptrs array is NULL");
    TEST_ASSERT_EQUAL(0, buzzer_off_r2d_fake.call_count);
}

void test_buzzer_api_init_null_tick_array(void) {
    buzzer_on_callback ons[BUZZER_TYPE_COUNT] = { buzzer_on_r2d, buzzer_on_assi };
    buzzer_off_callback offs[BUZZER_TYPE_COUNT] = { buzzer_off_r2d, buzzer_off_assi };
    buzzer_delay_callback syncs[BUZZER_TYPE_COUNT] = { buzzer_sync_r2d, buzzer_sync_assi };

    enum BuzzerReturnCode rc = buzzer_api_init(ons, offs, syncs, NULL);

    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, rc, "Init should fail if get_tick_ptrs array is NULL");
    TEST_ASSERT_EQUAL(0, buzzer_off_r2d_fake.call_count);
}

void test_buzzer_api_init_all_params_null(void) {
    enum BuzzerReturnCode rc = buzzer_api_init(NULL, NULL, NULL, NULL);

    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, rc, "Init must fail if all array pointers are NULL");

    // verify no hardware was touched
    TEST_ASSERT_EQUAL(0, buzzer_off_r2d_fake.call_count);
    TEST_ASSERT_EQUAL(0, buzzer_off_assi_fake.call_count);
}

void test_buzzer_api_init_null_buzzer_on_callback(void) {
    // provide a NULL only in the 'on' array for one of the buzzers
    buzzer_on_callback ons[BUZZER_TYPE_COUNT] = { NULL, buzzer_on_assi };
    buzzer_off_callback offs[BUZZER_TYPE_COUNT] = { buzzer_off_r2d, buzzer_off_assi };
    buzzer_delay_callback syncs[BUZZER_TYPE_COUNT] = { buzzer_sync_r2d, buzzer_sync_assi };
    buzzer_tick_callback ticks[BUZZER_TYPE_COUNT] = { get_tick_r2d, get_tick_assi };

    enum BuzzerReturnCode rc = buzzer_api_init(ons, offs, syncs, ticks);

    TEST_ASSERT_EQUAL_MESSAGE(
        BUZZER_RC_ERROR,
        rc,
        "Init should fail when a 'buzzer on' callback in the array is NULL");

    // ensure that buzzer off has never been called by anyone
    TEST_ASSERT_EQUAL_MESSAGE(0, buzzer_off_r2d_fake.call_count, "R2D buzzer off should not be called if any 'on' pointer is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(0, buzzer_off_assi_fake.call_count, "ASSI buzzer off should not be called if any 'on' pointer is NULL");
}

void test_buzzer_api_init_null_buzzer_off_callback(void) {
    // provide a NULL only in the 'off' array for one of the buzzers
    buzzer_on_callback ons[BUZZER_TYPE_COUNT] = { buzzer_on_r2d, buzzer_on_assi };
    buzzer_off_callback offs[BUZZER_TYPE_COUNT] = { buzzer_off_r2d, NULL };
    buzzer_delay_callback syncs[BUZZER_TYPE_COUNT] = { buzzer_sync_r2d, buzzer_sync_assi };
    buzzer_tick_callback ticks[BUZZER_TYPE_COUNT] = { get_tick_r2d, get_tick_assi };

    enum BuzzerReturnCode rc = buzzer_api_init(ons, offs, syncs, ticks);

    TEST_ASSERT_EQUAL_MESSAGE(
        BUZZER_RC_ERROR,
        rc,
        "Init should fail when a 'buzzer off' callback in the array is NULL");

    // ensure that buzzer off has never been called by anyone
    TEST_ASSERT_EQUAL_MESSAGE(0, buzzer_off_r2d_fake.call_count, "R2D buzzer off should not be called if any 'off' pointer is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(0, buzzer_off_assi_fake.call_count, "ASSI buzzer off should not be called if any 'off' pointer is NULL");
}

void test_buzzer_api_init_null_buzzer_play_sync_callback(void) {
    // provide a NULL only in the 'play sync' array for one of the buzzers
    buzzer_on_callback ons[BUZZER_TYPE_COUNT] = { buzzer_on_r2d, buzzer_on_assi };
    buzzer_off_callback offs[BUZZER_TYPE_COUNT] = { buzzer_off_r2d, buzzer_off_assi };
    buzzer_delay_callback syncs[BUZZER_TYPE_COUNT] = { buzzer_sync_r2d, NULL };
    buzzer_tick_callback ticks[BUZZER_TYPE_COUNT] = { get_tick_r2d, get_tick_assi };

    enum BuzzerReturnCode rc = buzzer_api_init(ons, offs, syncs, ticks);

    TEST_ASSERT_EQUAL_MESSAGE(
        BUZZER_RC_ERROR,
        rc,
        "Init should fail when a 'buzzer sync' callback in the array is NULL");

    // ensure that buzzer off has never been called by anyone
    TEST_ASSERT_EQUAL_MESSAGE(0, buzzer_off_r2d_fake.call_count, "R2D buzzer off should not be called if any 'sync' pointer is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(0, buzzer_off_assi_fake.call_count, "ASSI buzzer off should not be called if any 'sync' pointer is NULL");
}

void test_buzzer_api_init_null_buzzer_get_tick_callback(void) {
    // provide a NULL only in the 'off' array for one of the buzzers
    buzzer_on_callback ons[BUZZER_TYPE_COUNT] = { buzzer_on_r2d, buzzer_on_assi };
    buzzer_off_callback offs[BUZZER_TYPE_COUNT] = { buzzer_off_r2d, buzzer_off_assi };
    buzzer_delay_callback syncs[BUZZER_TYPE_COUNT] = { buzzer_sync_r2d, buzzer_sync_assi };
    buzzer_tick_callback ticks[BUZZER_TYPE_COUNT] = { NULL, get_tick_assi };

    enum BuzzerReturnCode rc = buzzer_api_init(ons, offs, syncs, ticks);

    TEST_ASSERT_EQUAL_MESSAGE(
        BUZZER_RC_ERROR,
        rc,
        "Init should fail when a 'buzzer get tick' callback in the array is NULL");

    // ensure that buzzer off has never been called by anyone
    TEST_ASSERT_EQUAL_MESSAGE(0, buzzer_off_r2d_fake.call_count, "R2D buzzer off should not be called if any 'get tick' pointer is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(0, buzzer_off_assi_fake.call_count, "ASSI buzzer off should not be called if any 'get tick' pointer is NULL");
}

void test_buzzer_api_init_failed_reset(void) {
    // only R2D fails buzzer off
    buzzer_off_r2d_fake.return_val = BUZZER_RC_ERROR;
    buzzer_off_assi_fake.return_val = BUZZER_RC_OK;

    buzzer_on_callback ons[BUZZER_TYPE_COUNT] = { buzzer_on_r2d, buzzer_on_assi };
    buzzer_off_callback offs[BUZZER_TYPE_COUNT] = { buzzer_off_r2d, buzzer_off_assi };
    buzzer_delay_callback syncs[BUZZER_TYPE_COUNT] = { buzzer_sync_r2d, buzzer_sync_assi };
    buzzer_tick_callback ticks[BUZZER_TYPE_COUNT] = { get_tick_r2d, get_tick_assi };

    enum BuzzerReturnCode rc = buzzer_api_init(ons, offs, syncs, ticks);

    // overall init must return error becasue r2d failed to turn off the buzzer
    TEST_ASSERT_EQUAL_MESSAGE(
        BUZZER_RC_ERROR,
        rc,
        "Initialization should report ERROR if at least one hardware off call fails");

    // verify assi buzzer off was still called despite r2d failing
    TEST_ASSERT_EQUAL_MESSAGE(
        1,
        buzzer_off_r2d_fake.call_count,
        "Buzzer R2D off should have been attempted");

    TEST_ASSERT_EQUAL_MESSAGE(
        1,
        buzzer_off_assi_fake.call_count,
        "Buzzer ASSI off MUST be attempted even if R2D fails");
}

void test_buzzer_api_init_initial_state(void) {
    buzzer_on_callback ons[BUZZER_TYPE_COUNT] = { buzzer_on_r2d, buzzer_on_assi };
    buzzer_off_callback offs[BUZZER_TYPE_COUNT] = { buzzer_off_r2d, buzzer_off_assi };
    buzzer_delay_callback syncs[BUZZER_TYPE_COUNT] = { buzzer_sync_r2d, buzzer_sync_assi };
    buzzer_tick_callback ticks[BUZZER_TYPE_COUNT] = { get_tick_r2d, get_tick_assi };

    buzzer_api_init(ons, offs, syncs, ticks);

    // check all buzzer handlers
    // R2D
    struct BuzzerHandler expected_r2d = {
        .buzzer_on = buzzer_on_r2d,
        .buzzer_off = buzzer_off_r2d,
        .buzzer_play_sync = buzzer_sync_r2d,
        .buzzer_get_tick = get_tick_r2d
        // other fields like .amplitude, are implicitly 0
    };

    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
        &expected_r2d,
        &buzzer_handlers[0],
        sizeof(struct BuzzerHandler),
        "R2D handler memory state is incorrect");

    // ASSI
    struct BuzzerHandler expected_assi = {
        .buzzer_on = buzzer_on_assi,
        .buzzer_off = buzzer_off_assi,
        .buzzer_play_sync = buzzer_sync_assi,
        .buzzer_get_tick = get_tick_assi
    };

    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
        &expected_assi,
        &buzzer_handlers[1],
        sizeof(struct BuzzerHandler),
        "ASSI handler memory state is incorrect");
}
/*! \} */

/*!
 * \defgroup buzzer_api_get_duration Tests for buzzer_api_get_duration function
 * \{
 */

void test_buzzer_api_get_duration_unknown_type(void) {
    uint32_t duration = buzzer_api_get_duration(99);

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0U, duration, "Getter should return 0 when an unknown buzzer type is requested.");
}

void test_buzzer_api_get_duration_known_type(void) {
    uint32_t expected_duration = 1000;
    buzzer_handlers[BUZZER_TYPE_R2D].duration = expected_duration;

    uint32_t duration = buzzer_api_get_duration(BUZZER_TYPE_R2D);

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(expected_duration, duration, "Getter should return the latest duration of the given buzzer type.");
}

/*! \} */

/*!
 * \defgroup buzzer_api_get_frequency Tests for buzzer_api_get_frequency function
 * \{
 */

void test_buzzer_api_get_frequency_unknown_type(void) {
    uint32_t frequency = buzzer_api_get_frequency(99);
    TEST_ASSERT_EQUAL_MESSAGE(0U, frequency, "Getter should return 0 when an unknown buzzer type is requested.");
}

void test_buzzer_api_get_frequency_known_type(void) {
    uint32_t expected_frequency = 500;
    buzzer_handlers[BUZZER_TYPE_R2D].frequency = expected_frequency;

    uint32_t frequency = buzzer_api_get_frequency(BUZZER_TYPE_R2D);

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(expected_frequency, frequency, "Getter should return the latest frequency of the given buzzer type.");
}

/*! \} */

/*!
 * \defgroup buzzer_api_get_amplitude Tests for buzzer_api_get_amplitude function
 * \{
 */

void test_buzzer_api_get_amplitude_unknown_type(void) {
    float amplitude = buzzer_api_get_amplitude(99);
    TEST_ASSERT_EQUAL_MESSAGE(0.0f, amplitude, "Getter should return 0.0 when an unknown buzzer type is requested.");
}

void test_buzzer_api_get_amplitude_known_type(void) {
    float expected_amplitude = 0.5f;
    buzzer_handlers[BUZZER_TYPE_R2D].amplitude = 0.5f;

    float amplitude = buzzer_api_get_amplitude(BUZZER_TYPE_R2D);

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(expected_amplitude, amplitude, "Getter should return the latest amplitude of the given buzzer type.");
}

/*! \} */

/*!
 * \defgroup buzzer_api_set_duration Tests for buzzer_api_set_duration function
 * \{
 */

void test_buzzer_api_set_duration_unknown_type(void) {
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_set_duration(99, 2000), "It is not possible to set a duration for an unknown buzzer type.");
    TEST_ASSERT_EQUAL_MESSAGE(0U, buzzer_handlers[BUZZER_TYPE_R2D].duration, "Duration should be left unchanged if previous call fails.");
}

void test_buzzer_api_set_duration_positive_duration(void) {
    uint32_t duration_ms = 4750;
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, buzzer_api_set_duration(BUZZER_TYPE_R2D, duration_ms), "If the buzzer type is known, it should be possible to change the duration.");
    TEST_ASSERT_EQUAL_MESSAGE(duration_ms, buzzer_handlers[BUZZER_TYPE_R2D].duration, "The duration should be exactly the one specified");
}

void test_buzzer_api_set_duration_negative_duration(void) {
    int32_t negative_duration = -1;
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, buzzer_api_set_duration(BUZZER_TYPE_R2D, (uint32_t)negative_duration), "The API should accept a value casted to the expected type.");
    TEST_ASSERT_EQUAL_MESSAGE((uint32_t)negative_duration, buzzer_handlers[BUZZER_TYPE_R2D].duration, "The duration should be exactly the one specified casted");
}

/*! \} */

/*!
 * \defgroup buzzer_api_set_frequency Tests for buzzer_api_set_frequency function
 * \{
 */

void test_buzzer_api_set_frequency_unknown_type(void) {
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_set_frequency(99, 1000), "It should not be possible to set a frequency for an unknown buzzer type.");
    TEST_ASSERT_EQUAL_MESSAGE(0U, buzzer_handlers[BUZZER_TYPE_R2D].frequency, "Frequency should be left unchanged if previous call fails.");
}

void test_buzzer_api_set_frequency_positive_frequency(void) {
    uint32_t frequency_hz = 2500;
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, buzzer_api_set_frequency(BUZZER_TYPE_R2D, frequency_hz), "If the buzzer type is known, it should be possible to change the frequency.");
    TEST_ASSERT_EQUAL_MESSAGE(frequency_hz, buzzer_handlers[BUZZER_TYPE_R2D].frequency, "The frequency should be exactly the one specified");
}

void test_buzzer_api_set_frequency_negative_frequency(void) {
    int32_t negative_freq = -440;
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, buzzer_api_set_frequency(BUZZER_TYPE_R2D, (uint32_t)negative_freq), "The API should accept a value casted to the expected type.");
    TEST_ASSERT_EQUAL_MESSAGE((uint32_t)negative_freq, buzzer_handlers[BUZZER_TYPE_R2D].frequency, "The frequency should be exactly the one specified casted");
}

/*! \} */

/*!
 * \defgroup buzzer_api_set_amplitude Tests for buzzer_api_set_amplitude function
 * \{
 */

void test_buzzer_api_set_amplitude_unknown_type(void) {
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_set_amplitude(99, 0.5f), "It should not be possible to set an amplitude for an unknown buzzer type.");
    TEST_ASSERT_EQUAL_MESSAGE(0.0f, buzzer_handlers[BUZZER_TYPE_R2D].amplitude, "Amplitude should be left unchanged if previous call fails.");
}

void test_buzzer_api_set_amplitude_in_range(void) {
    float amplitude = 0.75f;
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, buzzer_api_set_amplitude(BUZZER_TYPE_R2D, amplitude), "Amplitudes between 0.0 and 1.0 should be accepted.");
    TEST_ASSERT_EQUAL_MESSAGE(amplitude, buzzer_handlers[BUZZER_TYPE_R2D].amplitude, "Amplitude should be changed to the indicated value if previous call completes.");
}

void test_buzzer_api_set_amplitude_lower_value(void) {
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, buzzer_api_set_amplitude(BUZZER_TYPE_R2D, 0.0f), "The boundary value 0.0 should be accepted.");
    TEST_ASSERT_EQUAL_MESSAGE(0.0f, buzzer_handlers[BUZZER_TYPE_R2D].amplitude, "Amplitude should be changed to the indicated value if previous call completes.");
}

void test_buzzer_api_set_amplitude_upper_value(void) {
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, buzzer_api_set_amplitude(BUZZER_TYPE_R2D, 1.0f), "The boundary value 1.0 should be accepted.");
    TEST_ASSERT_EQUAL_MESSAGE(1.0f, buzzer_handlers[BUZZER_TYPE_R2D].amplitude, "Amplitude should be changed to the indicated value if previous call completes.");
}

void test_buzzer_api_set_amplitude_lower_value_out_of_range(void) {
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_set_amplitude(BUZZER_TYPE_R2D, -0.01f), "Values greater than 1.0 should return an error.");
    TEST_ASSERT_EQUAL_MESSAGE(0.0f, buzzer_handlers[BUZZER_TYPE_R2D].amplitude, "Amplitude should be changed to the indicated value if previous call completes.");
}

void test_buzzer_api_set_amplitude_upper_value_out_of_range(void) {
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_set_amplitude(BUZZER_TYPE_R2D, 1.01f), "Negative values should return an error.");
    TEST_ASSERT_EQUAL_MESSAGE(0.0f, buzzer_handlers[BUZZER_TYPE_R2D].amplitude, "Amplitude should be changed to the indicated value if previous call completes.");
}

/*! \} */

/*!
 * \defgroup buzzer_api_play_sync Tests for buzzer_api_play_sync function
 * \{
 */

void test_buzzer_api_play_sync_unknown_type(void) {
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_play_sync(99), "Should return an error immediately if the buzzer type is invalid.");
}

void test_buzzer_api_play_sync_callback_error(void) {
    // Force the mocked callback to return an error
    buzzer_sync_r2d_fake.return_val = BUZZER_RC_ERROR;

    // We expect the function to return the error code from the callback
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_play_sync(BUZZER_TYPE_R2D), "The return code should be ERROR as the callback returned an error itself");
}

void test_buzzer_api_play_sync_verifies_hardware_call(void) {
    // Setup values
    uint32_t test_freq = 1000;
    float test_amp = 0.8f;
    uint32_t test_duration = 500;

    buzzer_api_set_frequency(BUZZER_TYPE_R2D, test_freq);
    buzzer_api_set_amplitude(BUZZER_TYPE_R2D, test_amp);
    buzzer_api_set_duration(BUZZER_TYPE_R2D, test_duration);

    // Reset the fake to ensure a clean call count
    buzzer_sync_r2d_fake.return_val = BUZZER_RC_OK;

    enum BuzzerReturnCode rc = buzzer_api_play_sync(BUZZER_TYPE_R2D);

    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, rc, "Return code do not match. It should return OK");
    TEST_ASSERT_EQUAL_MESSAGE(1, buzzer_sync_r2d_fake.call_count, "The sync callback should be called exactly once.");

    // Verify parameters passed to the callback match what we set
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(test_freq, buzzer_sync_r2d_fake.arg0_val, "The passed frequency is not the correct one");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(test_amp, buzzer_sync_r2d_fake.arg1_val, "The passed amplitude is not the correct one");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(test_duration, buzzer_sync_r2d_fake.arg2_val, "The passed duration is not the correct one");
}

/*! \} */

/*!
 * \defgroup buzzer_api_request_poll Tests for the request/poll one-shot model
 * \{
 */

void test_buzzer_api_request_unknown_type(void) {
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_request(99), "Request should fail for unknown buzzer types.");
}

void test_buzzer_api_poll_unknown_type(void) {
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_poll(99), "Poll should fail for unknown buzzer types.");
}

void test_buzzer_api_get_play_state_unknown_type(void) {
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_PLAY_STATE_IDLE, buzzer_api_get_play_state(99), "Unknown types should report IDLE.");
}

void test_buzzer_api_poll_idle_is_noop(void) {
    // Nothing requested: polling must not touch the hardware and stay IDLE.
    enum BuzzerReturnCode rc = buzzer_api_poll(BUZZER_TYPE_ASSI);

    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, rc, "Polling an idle buzzer should return OK.");
    TEST_ASSERT_EQUAL_MESSAGE(0, buzzer_on_assi_fake.call_count, "Polling an idle buzzer must not turn it on.");
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_PLAY_STATE_IDLE, buzzer_api_get_play_state(BUZZER_TYPE_ASSI), "State should remain IDLE.");
}

void test_buzzer_api_request_marks_requested_without_hardware(void) {
    enum BuzzerReturnCode rc = buzzer_api_request(BUZZER_TYPE_ASSI);

    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, rc, "Request should succeed for a valid type.");
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_PLAY_STATE_REQUESTED, buzzer_api_get_play_state(BUZZER_TYPE_ASSI), "State should be REQUESTED after request().");
    TEST_ASSERT_EQUAL_MESSAGE(0, buzzer_on_assi_fake.call_count, "request() must not touch the hardware itself.");
}

void test_buzzer_api_poll_starts_after_request(void) {
    buzzer_api_set_duration(BUZZER_TYPE_ASSI, 100);
    buzzer_api_request(BUZZER_TYPE_ASSI);

    get_tick_assi_fake.return_val = 1000;
    enum BuzzerReturnCode rc = buzzer_api_poll(BUZZER_TYPE_ASSI);

    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_PLAYING, rc, "First poll after a request should start playing.");
    TEST_ASSERT_EQUAL_MESSAGE(1, buzzer_on_assi_fake.call_count, "First poll should turn the buzzer on exactly once.");
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_PLAY_STATE_PLAYING, buzzer_api_get_play_state(BUZZER_TYPE_ASSI), "State should be PLAYING after the first poll.");
}

void test_buzzer_api_poll_keeps_playing_before_duration(void) {
    buzzer_api_set_duration(BUZZER_TYPE_ASSI, 100);
    buzzer_api_request(BUZZER_TYPE_ASSI);

    get_tick_assi_fake.return_val = 1000;
    buzzer_api_poll(BUZZER_TYPE_ASSI);

    get_tick_assi_fake.return_val = 1050; // 50ms < 100ms duration
    enum BuzzerReturnCode rc = buzzer_api_poll(BUZZER_TYPE_ASSI);

    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_PLAYING, rc, "Polling before the duration elapses should keep PLAYING.");
    TEST_ASSERT_EQUAL_MESSAGE(1, buzzer_on_assi_fake.call_count, "The buzzer must not be re-triggered while playing.");
    TEST_ASSERT_EQUAL_MESSAGE(0, buzzer_off_assi_fake.call_count, "The buzzer must not be turned off before its duration.");
}

void test_buzzer_api_poll_finishes_and_latches_done(void) {
    buzzer_api_set_duration(BUZZER_TYPE_ASSI, 100);
    buzzer_api_request(BUZZER_TYPE_ASSI);

    get_tick_assi_fake.return_val = 1000;
    buzzer_api_poll(BUZZER_TYPE_ASSI);

    get_tick_assi_fake.return_val = 1100; // duration elapsed
    enum BuzzerReturnCode rc = buzzer_api_poll(BUZZER_TYPE_ASSI);

    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, rc, "Poll should return OK on the tick the sound finishes.");
    TEST_ASSERT_EQUAL_MESSAGE(1, buzzer_off_assi_fake.call_count, "The buzzer should be turned off once when finished.");
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_PLAY_STATE_DONE, buzzer_api_get_play_state(BUZZER_TYPE_ASSI), "State should latch DONE when finished.");
}

void test_buzzer_api_poll_does_not_restart_when_done(void) {
    buzzer_api_set_duration(BUZZER_TYPE_ASSI, 100);
    buzzer_api_request(BUZZER_TYPE_ASSI);

    get_tick_assi_fake.return_val = 1000;
    buzzer_api_poll(BUZZER_TYPE_ASSI); // start
    get_tick_assi_fake.return_val = 1100;
    buzzer_api_poll(BUZZER_TYPE_ASSI); // finish -> DONE

    // Keep polling well past the duration: it must stay DONE and never re-trigger.
    get_tick_assi_fake.return_val = 5000;
    enum BuzzerReturnCode rc = buzzer_api_poll(BUZZER_TYPE_ASSI);

    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, rc, "Polling a finished sound should return OK.");
    TEST_ASSERT_EQUAL_MESSAGE(1, buzzer_on_assi_fake.call_count, "A finished one-shot sound must NOT be auto-restarted by polling.");
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_PLAY_STATE_DONE, buzzer_api_get_play_state(BUZZER_TYPE_ASSI), "State should remain DONE.");
}

void test_buzzer_api_request_restarts_after_done(void) {
    buzzer_api_set_duration(BUZZER_TYPE_ASSI, 100);
    buzzer_api_request(BUZZER_TYPE_ASSI);
    get_tick_assi_fake.return_val = 1000;
    buzzer_api_poll(BUZZER_TYPE_ASSI); // start
    get_tick_assi_fake.return_val = 1100;
    buzzer_api_poll(BUZZER_TYPE_ASSI); // finish -> DONE

    // A fresh request restarts the sound from scratch.
    buzzer_api_request(BUZZER_TYPE_ASSI);
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_PLAY_STATE_REQUESTED, buzzer_api_get_play_state(BUZZER_TYPE_ASSI), "A new request should re-arm the play.");

    get_tick_assi_fake.return_val = 2000;
    enum BuzzerReturnCode rc = buzzer_api_poll(BUZZER_TYPE_ASSI);

    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_PLAYING, rc, "Poll after a new request should start playing again.");
    TEST_ASSERT_EQUAL_MESSAGE(2, buzzer_on_assi_fake.call_count, "The buzzer should be turned on again for the new play.");
}

void test_buzzer_api_reset_clears_play_state(void) {
    buzzer_api_set_duration(BUZZER_TYPE_ASSI, 100);
    buzzer_api_request(BUZZER_TYPE_ASSI);
    get_tick_assi_fake.return_val = 1000;
    buzzer_api_poll(BUZZER_TYPE_ASSI); // PLAYING

    buzzer_api_reset(BUZZER_TYPE_ASSI);

    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_PLAY_STATE_IDLE, buzzer_api_get_play_state(BUZZER_TYPE_ASSI), "Reset should return the play state to IDLE.");
}

/*! \} */

/*!
 * \defgroup buzzer_api_reset Test for buzzer_api_reset function
 * \{
 */

void test_buzzer_api_reset_unknown_type(void) {
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_reset(99), "Reset should fail for unknown buzzer types.");
}

void test_buzzer_api_reset_fails_if_hardware_fails(void) {
    // Force the hardware 'off' command to return an error
    buzzer_off_assi_fake.return_val = BUZZER_RC_ERROR;

    // The reset function should propagate this error
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_reset(BUZZER_TYPE_ASSI), "Reset should return ERROR if the hardware buzzer_off call fails.");
}

/*! \} */

int main(void) {
    UNITY_BEGIN();

    /*!
     * \addtogroup buzzer_api_init
     * \{
     */
    RUN_TEST(test_buzzer_api_init_null_on_array);
    RUN_TEST(test_buzzer_api_init_null_off_array);
    RUN_TEST(test_buzzer_api_init_null_sync_array);
    RUN_TEST(test_buzzer_api_init_null_tick_array);
    RUN_TEST(test_buzzer_api_init_all_params_null);
    RUN_TEST(test_buzzer_api_init_null_buzzer_on_callback);
    RUN_TEST(test_buzzer_api_init_null_buzzer_off_callback);
    RUN_TEST(test_buzzer_api_init_null_buzzer_play_sync_callback);
    RUN_TEST(test_buzzer_api_init_null_buzzer_get_tick_callback);
    RUN_TEST(test_buzzer_api_init_failed_reset);
    RUN_TEST(test_buzzer_api_init_initial_state);
    /*! \} */

    /*!
     * \addtogroup buzzer_api_get_duration
     * \{
     */
    RUN_TEST(test_buzzer_api_get_duration_unknown_type);
    RUN_TEST(test_buzzer_api_get_duration_known_type);
    /*! \} */

    /*!
     * \addtogroup buzzer_api_get_frequency
     * \{
     */
    RUN_TEST(test_buzzer_api_get_frequency_unknown_type);
    RUN_TEST(test_buzzer_api_get_frequency_known_type);
    /*! \} */

    /*!
     * \addtogroup buzzer_api_get_amplitude
     * \{
     */
    RUN_TEST(test_buzzer_api_get_amplitude_unknown_type);
    RUN_TEST(test_buzzer_api_get_amplitude_known_type);
    /*! \} */

    /*!
     * \addtogroup buzzer_api_set_duration
     * \{
     */
    RUN_TEST(test_buzzer_api_set_duration_unknown_type);
    RUN_TEST(test_buzzer_api_set_duration_positive_duration);
    RUN_TEST(test_buzzer_api_set_duration_negative_duration);
    /*! \} */

    /*!
     * \addtogroup buzzer_api_set_frequency
     * \{
     */
    RUN_TEST(test_buzzer_api_set_frequency_unknown_type);
    RUN_TEST(test_buzzer_api_set_frequency_positive_frequency);
    RUN_TEST(test_buzzer_api_set_frequency_negative_frequency);
    /*! \} */

    /*!
     * \addtogroup buzzer_api_set_amplitude
     * \{
     */
    RUN_TEST(test_buzzer_api_set_amplitude_unknown_type);
    RUN_TEST(test_buzzer_api_set_amplitude_in_range);
    RUN_TEST(test_buzzer_api_set_amplitude_lower_value);
    RUN_TEST(test_buzzer_api_set_amplitude_upper_value);
    RUN_TEST(test_buzzer_api_set_amplitude_lower_value_out_of_range);
    RUN_TEST(test_buzzer_api_set_amplitude_upper_value_out_of_range);
    /*! \} */

    /*!
     * \addtogroup buzzer_api_play_sync
     * \{
     */
    RUN_TEST(test_buzzer_api_play_sync_unknown_type);
    RUN_TEST(test_buzzer_api_play_sync_callback_error);
    RUN_TEST(test_buzzer_api_play_sync_verifies_hardware_call);
    /*! \} */

    /*!
     * \addtogroup buzzer_api_request_poll
     * \{
     */
    RUN_TEST(test_buzzer_api_request_unknown_type);
    RUN_TEST(test_buzzer_api_poll_unknown_type);
    RUN_TEST(test_buzzer_api_get_play_state_unknown_type);
    RUN_TEST(test_buzzer_api_poll_idle_is_noop);
    RUN_TEST(test_buzzer_api_request_marks_requested_without_hardware);
    RUN_TEST(test_buzzer_api_poll_starts_after_request);
    RUN_TEST(test_buzzer_api_poll_keeps_playing_before_duration);
    RUN_TEST(test_buzzer_api_poll_finishes_and_latches_done);
    RUN_TEST(test_buzzer_api_poll_does_not_restart_when_done);
    RUN_TEST(test_buzzer_api_request_restarts_after_done);
    RUN_TEST(test_buzzer_api_reset_clears_play_state);
    /*! \} */

    /*!
     * \defgroup buzzer_api_reset Test for buzzer_api_reset function
     * \{
     */
    RUN_TEST(test_buzzer_api_reset_unknown_type);
    RUN_TEST(test_buzzer_api_reset_fails_if_hardware_fails);
    /*! \} */

    return UNITY_END();
}
