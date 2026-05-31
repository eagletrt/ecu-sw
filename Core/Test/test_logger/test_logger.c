/*!
 * \file test_logger.c
 * \author Dorijan Di Zepp
 * \date 2026-06-01
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

    // initialize the logger
    logger_api_init(&pal_handler);

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
    enum LoggerReturnCode rc = logger_api_init(NULL);

    TEST_ASSERT_EQUAL_INT(LOGGER_RC_ERROR, rc);
}
/*! \} */

/*!
 * \defgroup logger_api_log Tests for logger_api_log function
 * \{
 */

void test_logger_api_log_should_fail_if_module_not_initialized(void) {
    // Force the global module handler into an uninitialized state by injecting NULL
    logger_handler.pal_handler = NULL;

    enum LoggerReturnCode rc = logger_api_log(LOGGER_LEVEL_INFO, "Test");

    TEST_ASSERT_EQUAL_INT(LOGGER_RC_ERROR, rc);
    // verify that the callback is not called
    TEST_ASSERT_EQUAL_INT(0, mock_uart_hardware_transmit_fake.call_count);
}

void test_logger_api_log_should_format_and_pass_successfully_to_hardware(void) {
    mock_uart_hardware_transmit_fake.return_val = PAL_RC_OK;

    enum LoggerReturnCode rc = logger_api_log(LOGGER_LEVEL_INFO, "Volt: %d", 12);

    TEST_ASSERT_EQUAL_INT(LOGGER_RC_OK, rc);
    TEST_ASSERT_EQUAL_INT(1, mock_uart_hardware_transmit_fake.call_count);

    // verify the message passed to the callback is the expected one
    const struct PalMessage *sent_msg = mock_uart_hardware_transmit_fake.arg0_val;
    TEST_ASSERT_NOT_NULL(sent_msg);
    TEST_ASSERT_EQUAL_STRING("[INFO]  Volt: 12", (const char *)sent_msg->payload);
}

void test_logger_api_log_should_bubble_error_if_hardware_fails(void) {
    mock_uart_hardware_transmit_fake.return_val = PAL_RC_IO_ERROR;

    enum LoggerReturnCode rc = logger_api_log(LOGGER_LEVEL_ERROR, "SDC Trip");

    TEST_ASSERT_EQUAL_INT(LOGGER_RC_ERROR, rc);
    TEST_ASSERT_EQUAL_INT(1, mock_uart_hardware_transmit_fake.call_count);
}

void test_logger_api_log_should_clamp_oversized_strings_at_max_msg_size(void) {
    mock_uart_hardware_transmit_fake.return_val = PAL_RC_OK;
    const char *long_msg = "This is a very long log trace that will definitely exceed sixty four bytes";

    enum LoggerReturnCode rc = logger_api_log(LOGGER_LEVEL_DEBUG, "%s", long_msg);

    TEST_ASSERT_EQUAL_INT(LOGGER_RC_OK, rc);
    TEST_ASSERT_EQUAL_INT(1, mock_uart_hardware_transmit_fake.call_count);

    // verify the callback received a message long exactly the maximum possible
    const struct PalMessage *sent_msg = mock_uart_hardware_transmit_fake.arg0_val;
    TEST_ASSERT_NOT_NULL(sent_msg);
    TEST_ASSERT_EQUAL_UINT32(TEST_UART_MAX_MSG_SIZE, sent_msg->size);
}
/*! \} */

int main(void) {
    UNITY_BEGIN();

    /*!
     * \addtogroup logger_api_init
     * \{
     */
    RUN_TEST(test_logger_api_init_should_fail_on_null_pointer);
    /*! \} */

    /*!
     * \addtogroup logger_api_log
     * \{
     */
    RUN_TEST(test_logger_api_log_should_fail_if_module_not_initialized);
    RUN_TEST(test_logger_api_log_should_format_and_pass_successfully_to_hardware);
    RUN_TEST(test_logger_api_log_should_bubble_error_if_hardware_fails);
    RUN_TEST(test_logger_api_log_should_clamp_oversized_strings_at_max_msg_size);
    /*! \} */

    return UNITY_END();
}