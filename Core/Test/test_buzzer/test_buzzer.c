/**
 * \file test_buzzer.c
 * \author Dorijan Di Zepp
 * \date 2026-04-07
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

// void test_buzzer_api_init_null_buzzer_on_callback(void) {
//     enum BuzzerReturnCode rc = buzzer_api_init(BUZZER_TYPE_R2D, NULL, buzzer_off_r2d, buzzer_sync_r2d, get_tick_r2d);

//     TEST_ASSERT_EQUAL_MESSAGE(
//         BUZZER_RC_ERROR,
//         rc,
//         "Init should fail when 'buzzer on' callback is NULL");

//     // check that buzzer off hasn't been called
//     TEST_ASSERT_EQUAL_MESSAGE(0, buzzer_off_r2d_fake.call_count, "Buzzer off should not be called when an unknown type is used");
// }

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
        // other fields like .amplitude, .is_playing are implicitly 0
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
 * \defgroup buzzer_api_is_playing Tests for buzzer_api_is_playing function
 * \{
 */

void test_buzzer_api_is_playing_unknown_type(void) {
    TEST_ASSERT_EQUAL_MESSAGE(false, buzzer_api_is_playing(99), "Should return false if an invalid buzzer type is provided.");
}

void test_buzzer_api_is_playing_known_type_idle(void) {
    buzzer_handlers[BUZZER_TYPE_R2D].is_playing = false;
    TEST_ASSERT_EQUAL_MESSAGE(false, buzzer_api_is_playing(BUZZER_TYPE_R2D), "Should return false if the buzzer is not playing.");
}

void test_buzzer_api_is_playing_known_type_playing(void) {
    buzzer_handlers[BUZZER_TYPE_R2D].is_playing = true;
    TEST_ASSERT_EQUAL_MESSAGE(true, buzzer_api_is_playing(BUZZER_TYPE_R2D), "Should return true if the buzzer is playing.");
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

void test_buzzer_api_play_sync_reset_flag_on_callback_error(void) {
    // Force the mocked callback to return an error
    buzzer_sync_r2d_fake.return_val = BUZZER_RC_ERROR;
    buzzer_api_play_sync(BUZZER_TYPE_R2D);

    TEST_ASSERT_FALSE_MESSAGE(buzzer_api_is_playing(BUZZER_TYPE_R2D), "The is_playing flag must be false even if the callback returns an error.");
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
 * \defgroup buzzer_api_play_async Tests buzzer_api_play_async function
 * \{
 */

void test_buzzer_api_play_async_unknown_type(void) {
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_play_async(99), "Should return ERROR for unknown buzzer types.");
}

void test_buzzer_api_play_async_handles_timer_overflow(void) {
    uint32_t duration = 100;
    buzzer_api_set_duration(BUZZER_TYPE_ASSI, duration);

    // Start just before overflow
    get_tick_assi_fake.return_val = 0xFFFFFFF0;
    buzzer_api_play_async(BUZZER_TYPE_ASSI);

    // Tick after overflow (e.g., 50ms later)
    get_tick_assi_fake.return_val = 0x00000022;

    // (0x22 - 0xFFFFFFF0) as uint32_t correctly equals 50
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_PLAYING, buzzer_api_play_async(BUZZER_TYPE_ASSI), "The buzzer should still play even in the case of a time overflow");
}

void test_buzzer_api_play_async_first_call(void) {
    uint32_t duration = 100;
    buzzer_api_set_duration(BUZZER_TYPE_ASSI, duration);

    get_tick_assi_fake.return_val = 1000; // Current time is 1000ms

    enum BuzzerReturnCode rc = buzzer_api_play_async(BUZZER_TYPE_ASSI);

    // Verify the buzzer is playing and that buzzer on has been called once
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_PLAYING, rc, "Return code do not match. It should return OK");
    TEST_ASSERT_EQUAL_MESSAGE(1, buzzer_on_assi_fake.call_count, "Callback on should have been called exactly one time");
    TEST_ASSERT_TRUE_MESSAGE(buzzer_api_is_playing(BUZZER_TYPE_ASSI), "The flag playing should be set to true");
}

void test_buzzer_api_play_async_playing_status(void) {
    uint32_t duration = 100;
    buzzer_api_set_duration(BUZZER_TYPE_ASSI, duration);

    get_tick_assi_fake.return_val = 1000; // Current time is 1000ms

    buzzer_api_play_async(BUZZER_TYPE_ASSI);

    // simulate half playing
    get_tick_assi_fake.return_val = 1050; // 50ms have passed (less than duration 100ms)

    enum BuzzerReturnCode rc = buzzer_api_play_async(BUZZER_TYPE_ASSI);

    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_PLAYING, rc, "Return code do not match. It should return PLAYING");
    TEST_ASSERT_EQUAL_MESSAGE(1, buzzer_on_assi_fake.call_count, "Callback on shouldn't be called again if buzzer is already playing"); // Should NOT call 'on' again
}

void test_buzzer_api_play_async_buzzer_stops(void) {
    uint32_t duration = 100;
    buzzer_api_set_duration(BUZZER_TYPE_ASSI, duration);

    get_tick_assi_fake.return_val = 1000; // Current time is 1000ms
    buzzer_api_play_async(BUZZER_TYPE_ASSI);

    // simulate pass of duration
    get_tick_assi_fake.return_val = 1100; // 100ms have passed

    enum BuzzerReturnCode rc = buzzer_api_play_async(BUZZER_TYPE_ASSI);

    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, rc, "Return code do not match");
    TEST_ASSERT_EQUAL_MESSAGE(1, buzzer_off_assi_fake.call_count, "Callback off should be called once when elapsed time is greater than the play duration");
    TEST_ASSERT_FALSE_MESSAGE(buzzer_api_is_playing(BUZZER_TYPE_ASSI), "If the buzzer stopped, the flag should be false");
}

void test_buzzer_api_play_async_params_unchanged_during_playback(void) {
    buzzer_api_set_duration(BUZZER_TYPE_ASSI, 100);

    // Start at 1000Hz / 0.5f
    buzzer_api_set_frequency(BUZZER_TYPE_ASSI, 1000);
    buzzer_api_set_amplitude(BUZZER_TYPE_ASSI, 0.5f);
    get_tick_assi_fake.return_val = 1000;

    buzzer_api_play_async(BUZZER_TYPE_ASSI); // Hardware gets 1000Hz, 0.5f

    // Update to new values mid-play
    buzzer_api_set_frequency(BUZZER_TYPE_ASSI, 2000);
    buzzer_api_set_amplitude(BUZZER_TYPE_ASSI, 0.9f);

    get_tick_assi_fake.return_val = 1050;
    buzzer_api_play_async(BUZZER_TYPE_ASSI);

    // Call count is still 1 (no re-trigger) and args are still the OLD ones
    TEST_ASSERT_EQUAL_MESSAGE(1, buzzer_on_assi_fake.call_count, "Callback one should be called once");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1000, buzzer_on_assi_fake.arg0_val, "Frequency should be left unchanged if buzzer is already playing");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.5f, buzzer_on_assi_fake.arg1_val, "Amplitude should be left unchanged if buzzer is already playing");
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

void test_buzzer_api_reset_stops_playing(void) {
    buzzer_api_set_duration(BUZZER_TYPE_ASSI, 500);
    buzzer_api_set_frequency(BUZZER_TYPE_ASSI, 1000);
    buzzer_api_set_amplitude(BUZZER_TYPE_ASSI, 0.5f);

    get_tick_assi_fake.return_val = 1000;
    buzzer_api_play_async(BUZZER_TYPE_ASSI);

    // Perform reset mid-play
    enum BuzzerReturnCode rc = buzzer_api_reset(BUZZER_TYPE_ASSI);

    // Verify hardware was turned off
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, rc, "Return code do not match. It should return OK");
    TEST_ASSERT_EQUAL_MESSAGE(1, buzzer_off_assi_fake.call_count, "Hardware should be silenced during reset.");

    // Verify internal state was zeroed
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, buzzer_api_get_frequency(BUZZER_TYPE_ASSI), "After reset, frequency should be zero");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, buzzer_api_get_duration(BUZZER_TYPE_ASSI), "After reset, duration should be zero");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, buzzer_api_get_amplitude(BUZZER_TYPE_ASSI), "After reset, amplitude should be zero");
    TEST_ASSERT_FALSE_MESSAGE(buzzer_api_is_playing(BUZZER_TYPE_ASSI), "After reset, playing flag should be false");
}

/*! \} */

int main(void) {
    UNITY_BEGIN();

    /*!
     * \addtogroup buzzer_api_init
     * \{
     */
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
     * \addtogroup buzzer_api_is_playing
     * \{
     */
    RUN_TEST(test_buzzer_api_is_playing_unknown_type);
    RUN_TEST(test_buzzer_api_is_playing_unknown_type);
    RUN_TEST(test_buzzer_api_is_playing_known_type_idle);
    RUN_TEST(test_buzzer_api_is_playing_known_type_playing);
    /*! \} */

    /*!
     * \addtogroup buzzer_api_play_sync
     * \{
     */
    RUN_TEST(test_buzzer_api_play_sync_unknown_type);
    RUN_TEST(test_buzzer_api_play_sync_callback_error);
    RUN_TEST(test_buzzer_api_play_sync_reset_flag_on_callback_error);
    RUN_TEST(test_buzzer_api_play_sync_verifies_hardware_call);
    /*! \} */

    /*!
     * \defgroup buzzer_api_play_async
     * \{
     */
    RUN_TEST(test_buzzer_api_play_async_unknown_type);
    RUN_TEST(test_buzzer_api_play_async_handles_timer_overflow);
    RUN_TEST(test_buzzer_api_play_async_first_call);
    RUN_TEST(test_buzzer_api_play_async_playing_status);
    RUN_TEST(test_buzzer_api_play_async_params_unchanged_during_playback);
    /*! \} */

    /*!
     * \defgroup buzzer_api_reset Test for buzzer_api_reset function
     * \{
     */
    RUN_TEST(test_buzzer_api_reset_unknown_type);
    RUN_TEST(test_buzzer_api_reset_fails_if_hardware_fails);
    RUN_TEST(test_buzzer_api_reset_stops_playing);
    /*! \} */

    return UNITY_END();
}