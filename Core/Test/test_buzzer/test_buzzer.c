/**
 * \file test_buzzer.c
 * \author Dorijan Di Zepp
 * \date 26-02-2026
 * \brief Unit tests for checking the correct behavrio of the buzzer module
 */

#include <unity.h>
#include "buzzer-api.h"
#include "eagletrt-api.h"

/* Mock state variables */
static int mock_on_count = 0;
static int mock_off_count = 0;
static uint32_t mock_system_ticks = 0;
static enum BuzzerReturnCode next_hw_return_status = BUZZER_RC_OK;

/* Mock implementations */
enum BuzzerReturnCode mock_buzzer_on(uint32_t frequency, float amplitude) {
    // both params are not used but are required to match function signature
    // this is the typical case for GPIO while for PWM, such parameters would be used
    // for the peripheral dynamic configuration
    EAGLETRT_API_UNUSED(frequency);
    EAGLETRT_API_UNUSED(amplitude);
    mock_on_count++;
    return next_hw_return_status;
}

enum BuzzerReturnCode mock_buzzer_off() {
    mock_off_count++;
    return next_hw_return_status;
}

void mock_play_sync(uint32_t ms) {
    mock_system_ticks += ms;
}

uint32_t mock_get_tick() {
    return mock_system_ticks;
}

// ------------------------------------------

void setUp(void) {
    mock_on_count = 0;
    mock_off_count = 0;
    mock_system_ticks = 0;
    next_hw_return_status = BUZZER_RC_OK;
}

void tearDown(void) {
    buzzer_api_reset();
}

/* Test cases */

/*!
 * \brief Ensures the module rejects NULL pointers for mandatory functions.
 */
void test_buzzer_init(void) {
    enum BuzzerReturnCode rc;

    rc = buzzer_api_init(NULL, mock_buzzer_off, mock_play_sync, mock_get_tick, 1000);
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, rc, "Initialization should fail when 'on' callback is NULL");

    rc = buzzer_api_init(mock_buzzer_on, NULL, mock_play_sync, mock_get_tick, 1000);
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, rc, "Initialization should fail when 'off' callback is NULL");

    rc = buzzer_api_init(mock_buzzer_on, mock_buzzer_off, NULL, mock_get_tick, 1000);
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, rc, "Initialization should fail when 'play sync' callback is NULL");

    rc = buzzer_api_init(mock_buzzer_on, mock_buzzer_off, mock_play_sync, NULL, 1000);
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, rc, "Initialization should fail when 'get_tick' callback is NULL");

    rc = buzzer_api_init(mock_buzzer_on, mock_buzzer_off, mock_play_sync, mock_get_tick, 4000);
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, rc, "Initialization should be successfull if all parameters are passed");
}

/*!
 * \brief Ensure that play in sync works if all parameters are available
 */
void test_buzzer_play_sync(void) {
    enum BuzzerReturnCode rc;

    // missing initialization will make fail the play
    rc = buzzer_api_play_sync();
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, rc, "Play sync should fail if callbacks are not correctly initialized");

    buzzer_api_init(mock_buzzer_on, mock_buzzer_off, mock_play_sync, mock_get_tick, BUZZER_RD2_SOUND_DURATION_MS);
    // at this point the handler have all values needed for playing in sync
    rc = buzzer_api_play_sync();
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, rc, "Play sync should complete after correct initialization");
    TEST_ASSERT_EQUAL_MESSAGE(false, buzzer_api_is_playing(), "The buzzer should not be playing after sync call");
}

/*!
 * \brief Verifies that sync mode respects hardware return codes.
 */
void test_buzzer_play_sync_hw_fails(void) {
    buzzer_api_init(mock_buzzer_on, mock_buzzer_off, mock_play_sync, mock_get_tick, 1000);

    next_hw_return_status = BUZZER_RC_ERROR;
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_play_sync(), "Play sync should fail if the callback failed to turn on the buzzer");
}

/*!
 * \brief Verify that the async play starts if all parameters are initialized corretly
 */
void test_buzzer_start_async(void) {
    enum BuzzerReturnCode rc;

    // missing initialization will make fail the play
    rc = buzzer_api_start_async();
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, rc, "Start async should fail if callbacks are not correctly initialized");

    // at this point the handler have all values needed for playing in sync
    buzzer_api_init(mock_buzzer_on, mock_buzzer_off, mock_play_sync, mock_get_tick, BUZZER_ASSI_SOUND_DURATION_MS);
    rc = buzzer_api_start_async();

    //check that both frequency and amplitude are set to 0 as they were not modified
    TEST_ASSERT_EQUAL_MESSAGE(0, buzzer_api_get_frequency(), "Frequency should be 0 if not modified after initialization");
    TEST_ASSERT_EQUAL_MESSAGE(0, buzzer_api_get_amplitude(), "Amplitude should be 0 if not modified after initialization");

    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, rc, "Start async should start after correct initialization");
    TEST_ASSERT_EQUAL_MESSAGE(true, buzzer_api_is_playing(), "The buzzer should be playing after async start");
}

/*!
 * \brief Verify that chaning frequency and amplitude can be done only if the buzzer is not
 * already playing and the new values are valid.
 */
void test_buzzer_set_frequency_and_amplitude(void) {
    enum BuzzerReturnCode rc;
    buzzer_api_init(mock_buzzer_on, mock_buzzer_off, mock_play_sync, mock_get_tick, BUZZER_ASSI_SOUND_DURATION_MS);
    buzzer_api_set_frequency(1000);
    buzzer_api_set_amplitude(0.5f);

    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(1000, buzzer_api_get_frequency(), "Frequency should be 1000 Hz");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.5f, buzzer_api_get_amplitude(), "Amplitude should be 0.5");

    // check that amplitude is not modified is it goes beyond limit
    buzzer_api_set_amplitude(1.2f);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.5f, buzzer_api_get_amplitude(), "Amplitude should be 0.5 and not 1.2 as it goes beyond the limit of 1.0");
    buzzer_api_set_amplitude(-0.1f);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.5f, buzzer_api_get_amplitude(), "Amplitude should be 0.5 and not 1.2 as it goes beyond the limit of 1.0");

    // check limit cases for amplitude
    buzzer_api_set_amplitude(0.0f);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, buzzer_api_get_amplitude(), "Amplitude should be 0.0");
    buzzer_api_set_amplitude(1.0f);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(1.0f, buzzer_api_get_amplitude(), "Amplitude should be 1.0");
}

/*!
 * \brief Ensures that if the hardware fails to turn ON, the module 
 * returns an error and does not set the internal state to 'playing'.
 */
void test_buzzer_start_async_hw_fails(void) {
    buzzer_api_init(mock_buzzer_on, mock_buzzer_off, mock_play_sync, mock_get_tick, BUZZER_ASSI_SOUND_DURATION_MS);

    next_hw_return_status = BUZZER_RC_ERROR; // force failure
    enum BuzzerReturnCode rc = buzzer_api_start_async();

    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, rc, "start_async should return error if hardware 'on' fails");
    // verify async_update doesn't think it's playing
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, buzzer_api_async_update(), "async_update should be skipped if start_async failed");
}

/*!
 * \brief Assure that the update function will stop the buzzer after the duration has elapsed
 */
void test_buzzer_api_async_update(void) {
    buzzer_api_init(mock_buzzer_on, mock_buzzer_off, mock_play_sync, mock_get_tick, 1000);
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, buzzer_api_async_update(), "Sync update should return OK as the buzzer is not playing");

    buzzer_api_start_async();
    TEST_ASSERT_TRUE_MESSAGE(buzzer_api_is_playing(), "Buzzer should be in playing state after start async");
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_PLAYING, buzzer_api_async_update(), "Sync update should return PLAYING as the buzzer is playing");

    //advance time manually in order to check if the next async update stops the buzzer
    mock_system_ticks = 2000;
    TEST_ASSERT_TRUE_MESSAGE(buzzer_api_is_playing(), "Even if time elapsed, without an update the buzzer has to still play");
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_OK, buzzer_api_async_update(), "Sync update should return OK to indicate that the buzzer have been stopped");
    TEST_ASSERT_FALSE_MESSAGE(buzzer_api_is_playing(), "After time elapsed and update has been called, the buzzer should not be playing");
}

/*!
 * \brief Assure that if the buzzer is currently playing, no state's attribute can be changed
 * unless the play duration elapses.
 */
void test_buzzer_state_unmodified_while_playing(void) {
    buzzer_api_init(mock_buzzer_on, mock_buzzer_off, mock_play_sync, mock_get_tick, 1000);

    //modify the parameters while buzzer is not playing
    buzzer_api_set_duration(2000);
    buzzer_api_set_frequency(5500);
    buzzer_api_set_amplitude(0.2);

    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(2000, buzzer_api_get_duration(), "Duration should have changed if buzzer is not playing");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(5500, buzzer_api_get_frequency(), "Frequency should have changed if buzzer is not playing");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.2, buzzer_api_get_amplitude(), "Amplitude should have changed if buzzer is not playing");

    buzzer_api_start_async();
    //modify the parameters while buzzer is playing
    buzzer_api_set_duration(1400);
    buzzer_api_set_frequency(4000);
    buzzer_api_set_amplitude(0.6);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(2000, buzzer_api_get_duration(), "Duration should not have changed if buzzer is not playing");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(5500, buzzer_api_get_frequency(), "Frequency should not have changed if buzzer is not playing");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.2, buzzer_api_get_amplitude(), "Amplitude should not have changed if buzzer is not playing");
}

/*!
 * \brief If buzzer init is called while a buzzer is active, it must turn off the 
 * previous hardware before switching to the new configuration.
 */
void test_buzzer_reinit_while_playing_stops_old_configuration(void) {
    buzzer_api_init(mock_buzzer_on, mock_buzzer_off, mock_play_sync, mock_get_tick, BUZZER_RD2_SOUND_DURATION_MS);
    buzzer_api_start_async();

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, mock_on_count, "Buzzer 'on' should have been called once");
    mock_off_count = 0; // reset for check

    // simulating a "mode" switch from R2D to ASSI
    buzzer_api_init(mock_buzzer_on, mock_buzzer_off, mock_play_sync, mock_get_tick, BUZZER_ASSI_SOUND_DURATION_MS);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, mock_off_count, "Re-initialization must stop active hardware");
    TEST_ASSERT_FALSE_MESSAGE(buzzer_api_is_playing(), "An init should always stop the previous 'configuration'");
    //check that module state is reset
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(BUZZER_ASSI_SOUND_DURATION_MS, buzzer_api_get_duration(), "Duration should have changed if buzzer is not playing");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0, buzzer_api_get_frequency(), "Frequency should have been cleared");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0, buzzer_api_get_amplitude(), "Amplitude should have been cleared");
}

/*!
 * \brief Ensures that calling clear forces the hardware off regardless of time.
 */
void test_buzzer_reset(void) {
    buzzer_api_init(mock_buzzer_on, mock_buzzer_off, mock_play_sync, mock_get_tick, 1000);
    buzzer_api_start_async();

    mock_off_count = 0;
    buzzer_api_reset();

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, mock_off_count, "Buzzer reset must immediately trigger hardware 'off'");
    TEST_ASSERT_EQUAL_MESSAGE(BUZZER_RC_ERROR, buzzer_api_async_update(), "Update after clear should return ERROR as callbacks are erased");
    //check that module state is reset
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0, buzzer_api_get_duration(), "Duration should have been cleared");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0, buzzer_api_get_frequency(), "Frequency should have been cleared");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0, buzzer_api_get_amplitude(), "Amplitude should have been cleared");
}

// -------------------------------

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_buzzer_init);
    RUN_TEST(test_buzzer_play_sync);
    RUN_TEST(test_buzzer_play_sync_hw_fails);
    RUN_TEST(test_buzzer_start_async);
    RUN_TEST(test_buzzer_set_frequency_and_amplitude);
    RUN_TEST(test_buzzer_start_async_hw_fails);
    RUN_TEST(test_buzzer_api_async_update);
    RUN_TEST(test_buzzer_state_unmodified_while_playing);
    RUN_TEST(test_buzzer_reinit_while_playing_stops_old_configuration);
    RUN_TEST(test_buzzer_reset);
    return UNITY_END();
}