#include <unity.h>
#include "buzzer.h"

/* Mock state variables */
static int mock_on_count = 0;
static int mock_off_count = 0;
static uint32_t mock_system_ticks = 0;
static enum BuzzerReturnCode next_hw_return_status = BUZZER_RC_OK;

/* Mock implementations */
enum BuzzerReturnCode mock_buzzer_on() {
    mock_on_count++;
    return next_hw_return_status;
}

enum BuzzerReturnCode mock_buzzer_off() {
    mock_off_count++;
    return next_hw_return_status;
}

uint32_t mock_get_tick() {
    return mock_system_ticks;
}

void mock_delay_callback(uint32_t ms) {
    mock_system_ticks += ms;
}

// ------------------------------------------

void setUp(void) {
    mock_on_count = 0;
    mock_off_count = 0;
    mock_system_ticks = 0;
    next_hw_return_status = BUZZER_RC_OK;
}

void tearDown(void) {
    buzzer_clear();
}

/* Test cases */

/**
 * Ensures the module rejects NULL pointers for mandatory functions.
 */
void test_buzzer_init_fails_with_null_pointers(void) {
    enum BuzzerReturnCode rc;

    rc = buzzer_init(NULL, mock_buzzer_off, mock_delay_callback, mock_get_tick, 1000);
    TEST_ASSERT_EQUAL(BUZZER_RC_ERROR, rc);

    rc = buzzer_init(mock_buzzer_on, NULL, mock_delay_callback, mock_get_tick, 1000);
    TEST_ASSERT_EQUAL(BUZZER_RC_ERROR, rc);

    rc = buzzer_init(mock_buzzer_on, mock_buzzer_off, NULL, mock_get_tick, 1000);
    TEST_ASSERT_EQUAL(BUZZER_RC_ERROR, rc);

    rc = buzzer_init(mock_buzzer_on, mock_buzzer_off, mock_delay_callback, NULL, 1000);
    TEST_ASSERT_EQUAL(BUZZER_RC_ERROR, rc);
}

/**
 * Ensures that if the hardware fails to turn ON, the module 
 * returns an error and does not set the internal state to 'playing'.
 */
void test_buzzer_start_fails_when_hw_fails(void) {
    buzzer_init(mock_buzzer_on, mock_buzzer_off, mock_delay_callback, mock_get_tick, BUZZER_ASSI_SOUND_DURATION);

    next_hw_return_status = BUZZER_RC_ERROR; // force failure
    enum BuzzerReturnCode rc = buzzer_start_async();

    TEST_ASSERT_EQUAL(BUZZER_RC_ERROR, rc);
    // verify async_update doesn't think it's playing
    TEST_ASSERT_EQUAL(BUZZER_RC_OK, buzzer_async_update());
}

/**
 * Verifies that duration updates are locked while the buzzer is active.
 */
void test_buzzer_duration_lock_during_playback(void) {
    buzzer_init(mock_buzzer_on, mock_buzzer_off, mock_delay_callback, mock_get_tick, 1000);

    buzzer_start_async();
    TEST_ASSERT_TRUE(buzzer_is_playing());

    // try to change duration while playing
    buzzer_set_duration(5000);

    // verify the change was ignored
    TEST_ASSERT_EQUAL_UINT32(1000, buzzer_get_duration());

    // advance time in order to check if the next update will be successful
    mock_system_ticks = 1500;
    TEST_ASSERT_EQUAL(BUZZER_RC_OK, buzzer_async_update());
    TEST_ASSERT_EQUAL_INT(1, mock_off_count);
    buzzer_set_duration(5000);
    TEST_ASSERT_EQUAL_UINT32(5000, buzzer_get_duration());
}

/**
 * If buzzer_init is called while a buzzer is active, it must turn off the 
 * previous hardware before switching to the new configuration.
 */
void test_buzzer_reinit_while_playing_stops_old_hw(void) {
    buzzer_init(mock_buzzer_on, mock_buzzer_off, mock_delay_callback, mock_get_tick, BUZZER_RD2_SOUND_DURATION);
    buzzer_start_async();

    TEST_ASSERT_EQUAL_INT(1, mock_on_count);
    mock_off_count = 0; // reset for check

    // simulating a mode switch from R2D to ASSI
    buzzer_init(mock_buzzer_on, mock_buzzer_off, mock_delay_callback, mock_get_tick, BUZZER_ASSI_SOUND_DURATION);

    TEST_ASSERT_EQUAL_INT(1, mock_off_count); // old hardware should have been cleared
}

/**
 * Verifies that sync mode respects hardware return codes.
 */
void test_buzzer_sync_fails_when_hw_fails(void) {
    buzzer_init(mock_buzzer_on, mock_buzzer_off, mock_delay_callback, mock_get_tick, 1000);

    next_hw_return_status = BUZZER_RC_ERROR;
    TEST_ASSERT_EQUAL(BUZZER_RC_ERROR, buzzer_play_sync());
}

/**
 * Ensures that calling clear forces the hardware off regardless of time.
 */
void test_buzzer_clear_forces_off(void) {
    buzzer_init(mock_buzzer_on, mock_buzzer_off, mock_delay_callback, mock_get_tick, 1000);
    buzzer_start_async();

    mock_off_count = 0;
    buzzer_clear();

    TEST_ASSERT_EQUAL_INT(1, mock_off_count);
    TEST_ASSERT_EQUAL(BUZZER_RC_OK, buzzer_async_update());
}

// -------------------------------

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_buzzer_init_fails_with_null_pointers);
    RUN_TEST(test_buzzer_start_fails_when_hw_fails);
    RUN_TEST(test_buzzer_duration_lock_during_playback);
    RUN_TEST(test_buzzer_reinit_while_playing_stops_old_hw);
    RUN_TEST(test_buzzer_sync_fails_when_hw_fails);
    RUN_TEST(test_buzzer_clear_forces_off);
    return UNITY_END();
}