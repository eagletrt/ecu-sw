/*!
 * \file test_logger.c
 * \author Dorijan Di Zepp
 * \date 2026-06-12
 * \brief Unit tests using FFF for testing the logger module.
 */

#include "unity.h"
#include "fff.h"
#include "logger-api.h"
#include "eagletrt-api.h"
#include "pal-api.h"
#include "arena-allocator-api.h"
#include <string.h>

extern struct LoggerHandler logger_handler;

// Global variables to manage a PAL usage
EAGLETRT_STATIC struct PalHandler pal_handler;
EAGLETRT_STATIC struct ArenaAllocatorHandler arena_allocator_handler;

// Configurations for PAL
#define TEST_UART_MAX_MSG_SIZE (64U)
#define TEST_RX_CAPACITY (10U)
#define TEST_TX_CAPACITY (10U)

DEFINE_FFF_GLOBALS;

// mocks for logger
FAKE_VALUE_FUNC(enum PalReturnCode, mock_uart_hardware_transmit, const struct PalMessage *);

void setUp(void) {
    // initialize the arena allocator to allocate pal
    arena_allocator_api_init(&arena_allocator_handler);

    pal_api_init(&pal_handler,
                 TEST_RX_CAPACITY,
                 TEST_TX_CAPACITY,
                 TEST_UART_MAX_MSG_SIZE,
                 NULL,
                 mock_uart_hardware_transmit,
                 NULL,
                 NULL,
                 &arena_allocator_handler);

    // initialize the logger with default enabled state
    logger_api_init(&pal_handler, LOGGER_STATE_ENABLED);

    // reset mock state
    RESET_FAKE(mock_uart_hardware_transmit);

    FFF_RESET_HISTORY();
}

/* --- Test Cases --- */

/*!
 * \defgroup logger_api_init Tests for logger_api_init function
 * \{
 */

void test_logger_api_init_should_fail_on_null_pointer(void) {
    logger_handler.pal_handler = NULL;

    enum LoggerReturnCode rc = logger_api_init(NULL, LOGGER_STATE_ENABLED);

    TEST_ASSERT_EQUAL_MESSAGE(LOGGER_RC_NULL_POINTER, rc, "Initialization loop failed to return NULL_POINTER when passed a null hardware binding");
    TEST_ASSERT_NULL_MESSAGE(logger_handler.pal_handler,
                             "Internal context handle pointer was corruptly initialized when passed a null value");
}

void test_logger_api_init_should_succeed_with_correct_pal_handler(void) {
    logger_handler.pal_handler = NULL;

    enum LoggerReturnCode rc = logger_api_init(&pal_handler, LOGGER_STATE_ENABLED);

    TEST_ASSERT_EQUAL_MESSAGE(LOGGER_RC_OK, rc, "Initialization loop failed to return OK when passed valid peripheral dependencies");
    TEST_ASSERT_EQUAL_PTR_MESSAGE(&pal_handler, logger_handler.pal_handler, "Internal module hardware interface address binding mismatch");
    TEST_ASSERT_EQUAL_MESSAGE(LOGGER_STATE_ENABLED, logger_handler.state, "Initial startup tracking state configuration mismatch");
}
/*! \} */

/*!
 * \defgroup logger_api_set_state Tests for logger_api_set_state function
 * \{
 */

void test_logger_api_set_state_should_successfully_change_to_disabled(void) {
    logger_api_init(&pal_handler, LOGGER_STATE_ENABLED);

    enum LoggerReturnCode rc = logger_api_set_state(LOGGER_STATE_DISABLED);

    TEST_ASSERT_EQUAL_MESSAGE(LOGGER_RC_OK, rc, "Setter failed to return OK when transitioning layout to DISABLED");
    TEST_ASSERT_EQUAL_MESSAGE(LOGGER_STATE_DISABLED, logger_handler.state, "Logger tracking state attribute was not updated to DISABLED");
}

void test_logger_api_set_state_should_successfully_change_to_enabled(void) {
    logger_api_init(&pal_handler, LOGGER_STATE_DISABLED);

    enum LoggerReturnCode rc = logger_api_set_state(LOGGER_STATE_ENABLED);

    TEST_ASSERT_EQUAL_MESSAGE(LOGGER_RC_OK, rc, "Setter failed to return OK when transitioning layout to ENABLED");
    TEST_ASSERT_EQUAL_MESSAGE(LOGGER_STATE_ENABLED, logger_handler.state, "Logger tracking state attribute was not updated to ENABLED");
}

void test_logger_api_set_state_should_fail_on_out_of_bounds_sentinel(void) {
    logger_api_init(&pal_handler, LOGGER_STATE_ENABLED);

    enum LoggerReturnCode rc = logger_api_set_state(LOGGER_STATE_COUNT);

    TEST_ASSERT_EQUAL_MESSAGE(LOGGER_RC_ERROR, rc, "Setter failed to reject out-of-bounds sentinel parameters with an ERROR return value");
    TEST_ASSERT_EQUAL_MESSAGE(LOGGER_STATE_ENABLED, logger_handler.state, "Logger tracking state was corrupted or overwritten by an invalid enum boundary entry");
}
/*! \} */

/*!
 * \defgroup logger_api_log Tests for logger_api_log function
 * \{
 */

void test_logger_api_log_should_fail_if_module_not_initialized(void) {
    logger_handler.pal_handler = NULL;

    enum LoggerReturnCode rc = logger_api_log(LOGGER_LEVEL_INFO, "Test Execution");

    TEST_ASSERT_EQUAL_MESSAGE(LOGGER_RC_NULL_POINTER, rc, "Logger did not crash cleanly or return NULL_POINTER when uninitialized downstream hooks were targeted");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, mock_uart_hardware_transmit_fake.call_count, "Hardware transmission loop triggered on a dangling uninitialized context structure");
}

void test_logger_api_log_should_silently_bypass_and_return_ok_when_disabled(void) {
    logger_api_init(&pal_handler, LOGGER_STATE_DISABLED);

    enum LoggerReturnCode rc = logger_api_log(LOGGER_LEVEL_ERROR, "Critical SDC Exception Code %d", 500);

    TEST_ASSERT_EQUAL_MESSAGE(LOGGER_RC_OK, rc, "Logger failed to return standard validation OK when processing log loops while explicitly disabled");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, mock_uart_hardware_transmit_fake.call_count, "Downstream physical transmission layer was targeted while logging execution was turned off");
}

void test_logger_api_log_should_format_and_pass_successfully_to_hardware(void) {
    mock_uart_hardware_transmit_fake.return_val = PAL_RC_OK;

    enum LoggerReturnCode rc = logger_api_log(LOGGER_LEVEL_INFO, "Volt: %d", 12);

    TEST_ASSERT_EQUAL_MESSAGE(LOGGER_RC_OK, rc, "Logger failed to successfully process a standard, correctly bounded string configuration");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, mock_uart_hardware_transmit_fake.call_count, "Logger failed to trigger downstream physical hardware abstraction transmission");

    const struct PalMessage *sent_msg = mock_uart_hardware_transmit_fake.arg0_val;
    TEST_ASSERT_NOT_NULL_MESSAGE(sent_msg, "Passed physical hardware message structure pointer evaluates to NULL");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("[INFO] Volt: 12", (const char *)sent_msg->payload, "Formatted metadata tag generation or payload sequence mismatch");
}

void test_logger_api_log_should_bubble_error_if_hardware_fails(void) {
    mock_uart_hardware_transmit_fake.return_val = PAL_RC_IO_ERROR;

    enum LoggerReturnCode rc = logger_api_log(LOGGER_LEVEL_ERROR, "SDC Trip Frame Detected");

    TEST_ASSERT_EQUAL_MESSAGE(LOGGER_RC_TRANSMISSION_ERROR, rc, "Logger failed to encapsulate downstream low-level hardware IO errors into generic transmission status codes");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, mock_uart_hardware_transmit_fake.call_count, "Downstream abstraction transmission handler was bypassed entirely when tracking fault parameters");
}

void test_logger_api_log_should_bubble_error_if_queue_overflows(void) {
    mock_uart_hardware_transmit_fake.return_val = PAL_RC_QUEUE_FULL;

    enum LoggerReturnCode rc = logger_api_log(LOGGER_LEVEL_WARN, "Queue Buffer Capacity Saturation Warning");

    TEST_ASSERT_EQUAL_MESSAGE(LOGGER_RC_BUFFER_FULL, rc, "Logger module failed to trap down-stream interface queue saturation limits cleanly");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, mock_uart_hardware_transmit_fake.call_count, "Downstream physical loop abstraction wasn't evaluated on buffer overflow triggers");
}

void test_logger_api_log_should_clamp_oversized_strings_at_max_msg_size(void) {
    mock_uart_hardware_transmit_fake.return_val = PAL_RC_OK;
    const char *long_msg = "This is a very long log trace that will definitely exceed sixty four bytes";

    enum LoggerReturnCode rc = logger_api_log(LOGGER_LEVEL_DEBUG, "%s", long_msg);

    TEST_ASSERT_EQUAL_MESSAGE(LOGGER_RC_OK, rc, "Logger component layout failed to return OK on runtime boundary truncation tasks");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, mock_uart_hardware_transmit_fake.call_count, "Downstream abstraction transmission handler bypassed when testing clamped buffer arrays");

    const struct PalMessage *sent_msg = mock_uart_hardware_transmit_fake.arg0_val;
    TEST_ASSERT_NOT_NULL_MESSAGE(sent_msg, "Targeted transmission pointer payload evaluates to NULL on truncation checks");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(TEST_UART_MAX_MSG_SIZE, sent_msg->size, "Logger processing execution did not match the physical tracking limits of the system array size definition");
}
/*! \} */

int main(void) {
    UNITY_BEGIN();

    /*!
     * \addtogroup logger_api_init
     * \{
     */
    RUN_TEST(test_logger_api_init_should_fail_on_null_pointer);
    RUN_TEST(test_logger_api_init_should_succeed_with_correct_pal_handler);
    /*! \} */

    /*!
     * \addtogroup logger_api_set_state
     * \{
     */
    RUN_TEST(test_logger_api_set_state_should_successfully_change_to_disabled);
    RUN_TEST(test_logger_api_set_state_should_successfully_change_to_enabled);
    RUN_TEST(test_logger_api_set_state_should_fail_on_out_of_bounds_sentinel);
    /*! \} */

    /*!
     * \addtogroup logger_api_log
     * \{
     */
    RUN_TEST(test_logger_api_log_should_fail_if_module_not_initialized);
    RUN_TEST(test_logger_api_log_should_silently_bypass_and_return_ok_when_disabled);
    RUN_TEST(test_logger_api_log_should_format_and_pass_successfully_to_hardware);
    RUN_TEST(test_logger_api_log_should_bubble_error_if_hardware_fails);
    RUN_TEST(test_logger_api_log_should_bubble_error_if_queue_overflows);
    RUN_TEST(test_logger_api_log_should_clamp_oversized_strings_at_max_msg_size);
    /*! \} */

    return UNITY_END();
}