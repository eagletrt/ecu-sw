/**
 * \file test_buzzer.c
 * \author Dorijan Di Zepp
 * \date 2026-03-19
 * \brief Unit tests using CMock for the buzzer module
 * \note Exhaustive testing of every buzzer instance (e.g R2D vs. ASSI) is 
 * unnecessary for most logic tests, as the API uses the buzzer type as a 
 * direct index into a common handler array; testing one instance validates 
 * the shared logic.
 */

#include <unity.h>
#include <stdbool.h>
#include "buzzer-api.h"
#include "fff.h"
#include "eagletrt-api.h"

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

    // initialize the API for both types
    buzzer_api_init(BUZZER_TYPE_R2D, buzzer_on_r2d, buzzer_off_r2d, buzzer_sync_r2d, get_tick_r2d, BUZZER_RD2_SOUND_DURATION_MS);
    buzzer_api_init(BUZZER_TYPE_ASSI, buzzer_on_assi, buzzer_off_assi, buzzer_sync_assi, get_tick_assi, BUZZER_ASSI_SOUND_DURATION_MS);
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

void test_buzzer_api_init_unknown_type(void) {
    // reset mock state to test number of calls
    RESET_FAKE(buzzer_off_r2d);

    TEST_ASSERT_EQUAL_MESSAGE(
        BUZZER_RC_ERROR,
        buzzer_api_init(99, buzzer_on_r2d, buzzer_off_r2d, buzzer_sync_r2d, get_tick_r2d, BUZZER_RD2_SOUND_DURATION_MS),
        "Initialization should fail when passing an unknown buzzer type");

    // check that buzzer off hasn't been called
    TEST_ASSERT_EQUAL_MESSAGE(0, buzzer_off_r2d_fake.call_count, "Buzzer off should not be called when an unknown type is used");
}

void test_buzzer_api_init_null_callbacks(void) {
    enum BuzzerReturnCode rc;

    rc = buzzer_api_init(BUZZER_TYPE_R2D, NULL, buzzer_off_r2d, buzzer_sync_r2d, get_tick_r2d, 1000);
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, rc, "Init should fail when 'buzzer on' callback is NULL");

    rc = buzzer_api_init(BUZZER_TYPE_R2D, buzzer_on_r2d, NULL, buzzer_sync_r2d, get_tick_r2d, 1000);
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, rc, "Init should fail when 'buzzer off' callback is NULL");

    rc = buzzer_api_init(BUZZER_TYPE_R2D, buzzer_on_r2d, buzzer_off_r2d, NULL, get_tick_r2d, 1000);
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, rc, "Init should fail when 'buzzer sync' callback is NULL");

    rc = buzzer_api_init(BUZZER_TYPE_R2D, buzzer_on_r2d, buzzer_off_r2d, buzzer_sync_r2d, NULL, 1000);
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, rc, "Init should fail when 'get tick' callback is NULL");
}

void test_buzzer_api_init_failed_reset(void) {
    // reset mock state to test number of calls
    RESET_FAKE(buzzer_off_r2d);

    buzzer_off_r2d_fake.return_val = BUZZER_RC_ERROR;
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_init(BUZZER_TYPE_R2D, buzzer_on_r2d, buzzer_off_r2d, buzzer_sync_r2d, get_tick_r2d, BUZZER_RD2_SOUND_DURATION_MS), "Initialization should fail if R2D off fails.");

    // check that buzzer off has been called but failed
    TEST_ASSERT_EQUAL_MESSAGE(1, buzzer_off_r2d_fake.call_count, "Buzzer off should have been called in order to return error");
}

void test_buzzer_api_init_call_off_once(void) {
    TEST_ASSERT_EQUAL_MESSAGE(1, buzzer_off_r2d_fake.call_count, "Buzzer off should have been called exactly one time during initialization.");
}

/*! \} */

/*!
 * \defgroup buzzer_api_get_duration Tests for buzzer_api_get_duration function
 * \{
 */

void test_buzzer_api_get_duration_unknown_type(void) {
    uint32_t duration;
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_get_duration(99, &duration), "Getter should return an error code when an unknown buzzer type is requested.");
}

void test_buzzer_api_get_duration_null_pointer(void) {
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_get_duration(BUZZER_TYPE_R2D, NULL), "Getter should return an error if the output pointer is NULL.");
}

/*! \} */

/*!
 * \defgroup buzzer_api_get_frequency Tests for buzzer_api_get_frequency function
 * \{
 */

void test_buzzer_api_get_frequency_unknown_type(void) {
    uint32_t frequency;
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_get_frequency(99, &frequency), "Getter should return an error code for invalid frequency requests.");
}

void test_buzzer_api_get_frequency_null_pointer(void) {
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_get_frequency(BUZZER_TYPE_R2D, NULL), "Getter should return an error if the output pointer is NULL.");
}

/*! \} */

/*!
 * \defgroup buzzer_api_get_amplitude Tests for buzzer_api_get_amplitude function
 * \{
 */

void test_buzzer_api_get_amplitude_unknown_type(void) {
    float amplitude;
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_get_amplitude(99, &amplitude), "Getter should return an error code for invalid amplitude requests.");
}

void test_buzzer_api_get_amplitude_null_pointer(void) {
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_get_amplitude(BUZZER_TYPE_R2D, NULL), "Getter should return an error if the output pointer is NULL.");
}

/*! \} */

/*!
 * \defgroup buzzer_api_set_duration Tests for buzzer_api_set_duration function
 * \{
 */

void test_buzzer_api_set_duration_unknown_type(void) {
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_set_duration(99, 2000), "It is not possible to set a duration for an unknown buzzer type.");
}

void test_buzzer_api_set_duration_positive_duration(void) {
    uint32_t duration_ms = 4750;
    uint32_t actual_duration = 0;

    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, buzzer_api_set_duration(BUZZER_TYPE_R2D, duration_ms), "If the buzzer type is known, it should be possible to change the duration.");

    TEST_ASSERT_EQUAL(BUZZER_RC_OK, buzzer_api_get_duration(BUZZER_TYPE_R2D, &actual_duration));
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(duration_ms, actual_duration, "The buzzer duration should correspond to the latest value passed.");
}

void test_buzzer_api_set_duration_negative_duration(void) {
    int32_t negative_duration = -1;
    uint32_t expected_wrap = (uint32_t)negative_duration;
    uint32_t actual_duration = 0;

    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, buzzer_api_set_duration(BUZZER_TYPE_R2D, (uint32_t)negative_duration), "The API should accept a value casted to the expected type.");

    TEST_ASSERT_EQUAL(BUZZER_RC_OK, buzzer_api_get_duration(BUZZER_TYPE_R2D, &actual_duration));
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(expected_wrap, actual_duration, "The stored value should be the large positive unsigned equivalent (underflow).");
}

/*! \} */

/*!
 * \defgroup buzzer_api_set_frequency Tests for buzzer_api_set_frequency function
 * \{
 */

void test_buzzer_api_set_frequency_unknown_type(void) {
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_set_frequency(99, 1000), "It should not be possible to set a frequency for an unknown buzzer type.");
}

void test_buzzer_api_set_frequency_positive_value(void) {
    uint32_t frequency_hz = 2500;
    uint32_t actual_frequency = 0;

    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, buzzer_api_set_frequency(BUZZER_TYPE_R2D, frequency_hz), "If the buzzer type is known, it should be possible to change the frequency.");

    TEST_ASSERT_EQUAL(BUZZER_RC_OK, buzzer_api_get_frequency(BUZZER_TYPE_R2D, &actual_frequency));
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(frequency_hz, actual_frequency, "The buzzer frequency should correspond to the latest value passed.");
}

void test_buzzer_api_set_frequency_negative_wrap_behavior(void) {
    int32_t negative_freq = -440;
    uint32_t expected_wrap = (uint32_t)negative_freq;
    uint32_t actual_frequency = 0;

    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, buzzer_api_set_frequency(BUZZER_TYPE_R2D, (uint32_t)negative_freq), "The API should accept a value casted to the expected type.");

    TEST_ASSERT_EQUAL(BUZZER_RC_OK, buzzer_api_get_frequency(BUZZER_TYPE_R2D, &actual_frequency));
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(expected_wrap, actual_frequency, "The stored frequency should be the large positive unsigned equivalent due to underflow.");
}

/*! \} */

/*!
 * \defgroup buzzer_api_set_amplitude Tests for buzzer_api_set_amplitude function
 * \{
 */

void test_buzzer_api_set_amplitude_unknown_type(void) {
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_set_amplitude(99, 0.5f), "It should not be possible to set an amplitude for an unknown buzzer type.");
}

void test_buzzer_api_set_amplitude_in_range(void) {
    float valid_amplitude = 0.75f;
    float actual_amplitude = 0.0f;

    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, buzzer_api_set_amplitude(BUZZER_TYPE_R2D, valid_amplitude), "Amplitudes between 0.0 and 1.0 should be accepted.");

    TEST_ASSERT_EQUAL(BUZZER_RC_OK, buzzer_api_get_amplitude(BUZZER_TYPE_R2D, &actual_amplitude));
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.001f, valid_amplitude, actual_amplitude, "The stored amplitude should match the valid value passed.");
}

void test_buzzer_api_set_amplitude_boundary_cases(void) {
    float actual_amplitude = 0.0f;

    // Testing the lower limit (0.0)
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, buzzer_api_set_amplitude(BUZZER_TYPE_R2D, 0.0f), "The boundary value 0.0 should be accepted.");

    TEST_ASSERT_EQUAL(BUZZER_RC_OK, buzzer_api_get_amplitude(BUZZER_TYPE_R2D, &actual_amplitude));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, actual_amplitude);

    // Testing the upper limit (1.0)
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, buzzer_api_set_amplitude(BUZZER_TYPE_R2D, 1.0f), "The boundary value 1.0 should be accepted.");

    TEST_ASSERT_EQUAL(BUZZER_RC_OK, buzzer_api_get_amplitude(BUZZER_TYPE_R2D, &actual_amplitude));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, actual_amplitude);
}

void test_buzzer_api_set_amplitude_out_of_range(void) {
    // Testing just above 1.0
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_set_amplitude(BUZZER_TYPE_R2D, 1.01f), "Values greater than 1.0 should return an error.");

    // Testing just below 0.0
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_set_amplitude(BUZZER_TYPE_R2D, -0.01f), "Negative values should return an error.");
}

/*! \} */

/*!
 * \defgroup buzzer_api_is_playing Tests for buzzer_api_is_playing function
 * \{
 */

void test_buzzer_api_is_playing_null_pointer(void) {
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_is_playing(BUZZER_TYPE_R2D, NULL), "The getter must return an error if the pointer is NULL.");
}

void test_buzzer_api_is_playing_unknown_type(void) {
    bool is_playing;
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_is_playing(99, &is_playing), "Should return an error if an invalid buzzer type is provided.");
}

/*! \} */

/*!
 * \defgroup buzzer_api_play_sync Tests for buzzer_api_play_sync function
 * \{
 */

void test_buzzer_api_play_sync_unknown_type(void) {
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_play_sync(99), "Should return an error immediately if the buzzer type is invalid.");
}

void test_buzzer_api_play_sync_is_playing_flag_reset_on_callback_error(void) {
    // Force the mocked callback to return an error
    buzzer_sync_r2d_fake.return_val = BUZZER_RC_ERROR;

    // We expect the function to return the error code from the callback
    // but crucially, the 'is_playing' flag must be false afterward.
    TEST_ASSERT_EQUAL(BUZZER_RC_ERROR, buzzer_api_play_sync(BUZZER_TYPE_R2D));

    bool is_playing;
    buzzer_api_is_playing(BUZZER_TYPE_R2D, &is_playing);
    TEST_ASSERT_FALSE_MESSAGE(is_playing, "The is_playing flag must be false even if the callback returns an error.");
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
    RESET_FAKE(buzzer_sync_r2d);
    buzzer_sync_r2d_fake.return_val = BUZZER_RC_OK;

    enum BuzzerReturnCode rc = buzzer_api_play_sync(BUZZER_TYPE_R2D);

    TEST_ASSERT_EQUAL(BUZZER_RC_OK, rc);
    TEST_ASSERT_EQUAL_MESSAGE(1, buzzer_sync_r2d_fake.call_count, "The sync callback should be called exactly once.");

    // Verify parameters passed to the callback match what we set
    TEST_ASSERT_EQUAL_UINT32(test_freq, buzzer_sync_r2d_fake.arg0_val);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, test_amp, buzzer_sync_r2d_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT32(test_duration, buzzer_sync_r2d_fake.arg2_val);
}

void test_buzzer_api_play_sync_multiple_calls(void) {
    RESET_FAKE(buzzer_sync_r2d);

    buzzer_api_play_sync(BUZZER_TYPE_R2D);
    buzzer_api_play_sync(BUZZER_TYPE_R2D);
    buzzer_api_play_sync(BUZZER_TYPE_R2D);

    TEST_ASSERT_EQUAL_MESSAGE(3, buzzer_sync_r2d_fake.call_count, "The sync callback should track multiple sequential calls correctly.");
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
    TEST_ASSERT_EQUAL(BUZZER_RC_PLAYING, buzzer_api_play_async(BUZZER_TYPE_ASSI));
}

void test_buzzer_api_play_async_update(void) {
    uint32_t duration = 100;
    buzzer_api_set_duration(BUZZER_TYPE_ASSI, duration);

    get_tick_assi_fake.return_val = 1000; // Current time is 1000ms
    RESET_FAKE(buzzer_on_assi);

    enum BuzzerReturnCode rc = buzzer_api_play_async(BUZZER_TYPE_ASSI);

    TEST_ASSERT_EQUAL(BUZZER_RC_PLAYING, rc);
    TEST_ASSERT_EQUAL(1, buzzer_on_assi_fake.call_count);

    // Verify flag via getter
    bool is_playing;
    buzzer_api_is_playing(BUZZER_TYPE_ASSI, &is_playing);
    TEST_ASSERT_TRUE(is_playing);

    // simulate half playing
    get_tick_assi_fake.return_val = 1050; // 50ms have passed (less than duration 100ms)

    rc = buzzer_api_play_async(BUZZER_TYPE_ASSI);

    TEST_ASSERT_EQUAL(BUZZER_RC_PLAYING, rc);
    TEST_ASSERT_EQUAL(1, buzzer_on_assi_fake.call_count); // Should NOT call 'on' again

    // simulate pass of duration
    get_tick_assi_fake.return_val = 1100; // 100ms have passed
    RESET_FAKE(buzzer_off_assi);

    rc = buzzer_api_play_async(BUZZER_TYPE_ASSI);

    TEST_ASSERT_EQUAL(BUZZER_RC_OK, rc);
    TEST_ASSERT_EQUAL(1, buzzer_off_assi_fake.call_count);

    buzzer_api_is_playing(BUZZER_TYPE_ASSI, &is_playing);
    TEST_ASSERT_FALSE(is_playing);
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
    TEST_ASSERT_EQUAL(1, buzzer_on_assi_fake.call_count);
    TEST_ASSERT_EQUAL_UINT32(1000, buzzer_on_assi_fake.arg0_val);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, buzzer_on_assi_fake.arg1_val);
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
    RESET_FAKE(buzzer_off_assi);
    enum BuzzerReturnCode rc = buzzer_api_reset(BUZZER_TYPE_ASSI);

    // Verify hardware was turned off
    TEST_ASSERT_EQUAL(BUZZER_RC_OK, rc);
    TEST_ASSERT_EQUAL_MESSAGE(1, buzzer_off_assi_fake.call_count, "Hardware should be silenced during reset.");

    // Verify internal state was zeroed
    uint32_t freq, dur;
    float amp;
    bool is_playing;

    buzzer_api_get_frequency(BUZZER_TYPE_ASSI, &freq);
    buzzer_api_get_duration(BUZZER_TYPE_ASSI, &dur);
    buzzer_api_get_amplitude(BUZZER_TYPE_ASSI, &amp);
    buzzer_api_is_playing(BUZZER_TYPE_ASSI, &is_playing);

    TEST_ASSERT_EQUAL(0, freq);
    TEST_ASSERT_EQUAL(0, dur);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, amp);
    TEST_ASSERT_FALSE(is_playing);
}

/*! \} */

int main(void) {
    UNITY_BEGIN();

    /*!
    * \addtogroup buzzer_api_init
    * \{
    */
    RUN_TEST(test_buzzer_api_init_unknown_type);
    RUN_TEST(test_buzzer_api_init_null_callbacks);
    RUN_TEST(test_buzzer_api_init_failed_reset);
    RUN_TEST(test_buzzer_api_init_call_off_once);
    /*! \} */

    /*!
    * \addtogroup buzzer_api_get_duration
    * \{
    */
    RUN_TEST(test_buzzer_api_get_duration_unknown_type);
    RUN_TEST(test_buzzer_api_get_duration_null_pointer);
    /*! \} */

    /*!
    * \addtogroup buzzer_api_get_frequency
    * \{
    */
    RUN_TEST(test_buzzer_api_get_frequency_unknown_type);
    RUN_TEST(test_buzzer_api_get_frequency_null_pointer);
    /*! \} */

    /*!
    * \addtogroup buzzer_api_get_amplitude
    * \{
    */
    RUN_TEST(test_buzzer_api_get_amplitude_unknown_type);
    RUN_TEST(test_buzzer_api_get_amplitude_null_pointer);
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
    RUN_TEST(test_buzzer_api_set_frequency_positive_value);
    RUN_TEST(test_buzzer_api_set_frequency_negative_wrap_behavior);
    /*! \} */

    /*!
    * \addtogroup buzzer_api_set_amplitude
    * \{
    */
    RUN_TEST(test_buzzer_api_set_amplitude_unknown_type);
    RUN_TEST(test_buzzer_api_set_amplitude_in_range);
    RUN_TEST(test_buzzer_api_set_amplitude_boundary_cases);
    RUN_TEST(test_buzzer_api_set_amplitude_out_of_range);
    /*! \} */

    /*!
    * \addtogroup buzzer_api_is_playing
    * \{
    */
    RUN_TEST(test_buzzer_api_is_playing_null_pointer);
    RUN_TEST(test_buzzer_api_is_playing_unknown_type);
    /*! \} */

    /*!
    * \addtogroup buzzer_api_play_sync
    * \{
    */
    RUN_TEST(test_buzzer_api_play_sync_unknown_type);
    RUN_TEST(test_buzzer_api_play_sync_is_playing_flag_reset_on_callback_error);
    RUN_TEST(test_buzzer_api_play_sync_verifies_hardware_call);
    RUN_TEST(test_buzzer_api_play_sync_multiple_calls);
    /*! \} */

    /*!
    * \defgroup buzzer_api_play_async
    * \{
    */
    RUN_TEST(test_buzzer_api_play_async_unknown_type);
    RUN_TEST(test_buzzer_api_play_async_handles_timer_overflow);
    RUN_TEST(test_buzzer_api_play_async_update);
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