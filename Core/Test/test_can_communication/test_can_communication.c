/*!
 * \file test_can_communication_api.c
 * \author Dorijan Di Zepp
 * \date 2026-06-13
 * \brief Unit tests using FFF for testing the CAN communication API module.
 */

#include "unity.h"
#include "fff.h"
#include "can-communication-api.h"
#include "eagletrt-api.h"
#include "pal-api.h"
#include "arena-allocator-api.h"
#include <string.h>

extern struct CanCommunicationHandler can_comm_handler;

// Global variables to manage PAL usage across the 3 nodes
EAGLETRT_STATIC struct PalHandler pal_can1;
EAGLETRT_STATIC struct PalHandler pal_can2;
EAGLETRT_STATIC struct PalHandler pal_can3;
EAGLETRT_STATIC struct ArenaAllocatorHandler arena_allocator_handler;

// Configurations for PAL
#define TEST_CAN_MAX_MSG_SIZE (12U) // 4 bytes for CAN ID + 8 bytes payload
#define TEST_RX_CAPACITY (10U)
#define TEST_TX_CAPACITY (10U)

DEFINE_FFF_GLOBALS;

// Mocks for CAN deserialization and sending
FAKE_VALUE_FUNC(enum PalReturnCode, mock_can_deserialize, const struct PalMessage *, void *);
FAKE_VALUE_FUNC(enum PalReturnCode, mock_can_send, const struct PalMessage *);

void setUp(void) {
    // initialize the arena allocator to allocate pal
    arena_allocator_api_init(&arena_allocator_handler);

    // initialize the 3 pal handlers representing the 3 CAN nodes
    pal_api_init(&pal_can1, TEST_RX_CAPACITY, TEST_TX_CAPACITY, TEST_CAN_MAX_MSG_SIZE, mock_can_deserialize, mock_can_send, NULL, NULL, &arena_allocator_handler);

    pal_api_init(&pal_can2, TEST_RX_CAPACITY, TEST_TX_CAPACITY, TEST_CAN_MAX_MSG_SIZE, mock_can_deserialize, mock_can_send, NULL, NULL, &arena_allocator_handler);

    pal_api_init(&pal_can3, TEST_RX_CAPACITY, TEST_TX_CAPACITY, TEST_CAN_MAX_MSG_SIZE, mock_can_deserialize, mock_can_send, NULL, NULL, &arena_allocator_handler);

    // initialize the can communication module
    can_communication_api_init(&pal_can1, &pal_can2, &pal_can3);

    // reset mock state
    RESET_FAKE(mock_can_deserialize);
    RESET_FAKE(mock_can_send);

    mock_can_deserialize_fake.return_val = PAL_RC_OK;
    mock_can_send_fake.return_val = PAL_RC_OK;

    FFF_RESET_HISTORY();
}

/* --- Test Cases --- */

/*!
 * \defgroup can_communication_api_init Tests for can_communication_api_init function
 * \{
 */

void test_can_communication_api_init_should_fail_on_null_pointer(void) {
    can_comm_handler.pal_can1 = NULL;
    can_comm_handler.pal_can2 = NULL;
    can_comm_handler.pal_can3 = NULL;

    enum CanCommunicationReturnCode rc = can_communication_api_init(NULL, &pal_can2, &pal_can3);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_NULL_POINTER, rc, "Initialization loop failed to return NULL_POINTER when passed a null hardware binding");
    TEST_ASSERT_NULL_MESSAGE(can_comm_handler.pal_can1, "Internal context handle pointer was corruptly initialized when passed a null value");
}

void test_can_communication_api_init_should_succeed_with_correct_pointers(void) {
    can_comm_handler.pal_can1 = NULL;
    can_comm_handler.pal_can2 = NULL;
    can_comm_handler.pal_can3 = NULL;

    enum CanCommunicationReturnCode rc = can_communication_api_init(&pal_can1, &pal_can2, &pal_can3);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_OK, rc, "Initialization loop failed to return OK when passed valid peripheral dependencies");
    TEST_ASSERT_EQUAL_PTR_MESSAGE(&pal_can1, can_comm_handler.pal_can1, "Internal module CAN1 hardware interface address binding mismatch");
    TEST_ASSERT_EQUAL_PTR_MESSAGE(&pal_can2, can_comm_handler.pal_can2, "Internal module CAN2 hardware interface address binding mismatch");
    TEST_ASSERT_EQUAL_PTR_MESSAGE(&pal_can3, can_comm_handler.pal_can3, "Internal module CAN3 hardware interface address binding mismatch");
}
/*! \} */

/*!
 * \defgroup can_communication_api_add_to_rx Tests for can_communication_api_add_to_rx function
 * \{
 */

void test_can_communication_api_add_to_rx_should_fail_on_invalid_node(void) {
    uint8_t payload[] = { 0x11, 0x22 };
    enum CanCommunicationReturnCode rc = can_communication_api_add_to_rx(CAN_COMM_NODE_COUNT, payload, sizeof(payload));

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_INVALID_ARGUMENT, rc, "Failed to trap invalid CAN node sentinel in RX queue routing");
}

void test_can_communication_api_add_to_rx_should_succeed(void) {
    uint8_t payload[] = { 0xAA, 0xBB, 0xCC };

    enum CanCommunicationReturnCode rc = can_communication_api_add_to_rx(CAN_COMM_NODE_1, payload, sizeof(payload));

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_OK, rc, "Failed to successfully route valid incoming CAN payload into PAL RX queue");
}

void test_can_communication_api_add_to_rx_should_fail_with_buffer_full(void) {
    uint8_t dummy_payload[] = { 0x11, 0x22 };
    enum CanCommunicationReturnCode rc = CAN_COMM_RC_OK;

    // 1. Fill the queue exactly to its maximum capacity limit
    for (uint32_t i = 0; i < TEST_RX_CAPACITY; i++) {
        rc = can_communication_api_add_to_rx(CAN_COMM_NODE_1, dummy_payload, sizeof(dummy_payload));
        TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_OK, rc, "Queue failed to accept inserts up to nominal capacity limits");
    }

    // 2. The N+1 insertion attempt must trigger a capacity saturation error
    rc = can_communication_api_add_to_rx(CAN_COMM_NODE_1, dummy_payload, sizeof(dummy_payload));
    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_BUFFER_FULL, rc, "Module failed to trap and return BUFFER_FULL on queue saturation");
}
/*! \} */

/*!
 * \defgroup can_communication_api_process_rx Tests for can_communication_api_process_rx function
 * \{
 */

void test_can_communication_api_process_rx_should_fail_if_uninitialized(void) {
    can_comm_handler.pal_can1 = NULL;
    uint32_t dummy_state = 0;

    enum CanCommunicationReturnCode rc = can_communication_api_process_rx(&dummy_state);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_NULL_POINTER, rc, "Process loop did not trap uninitialized CAN1 context");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, mock_can_deserialize_fake.call_count, "Deserialization callback triggered on a dangling context");
}

void test_can_communication_api_process_rx_should_trigger_deserialize_callback_with_provided_state(void) {
    // Arrange: Create a dummy tracking structure to act as our "global" or context state
    uint32_t mock_global_vehicle_state = 0xABCDEF;

    // Stage data into the real PAL queue
    uint8_t dummy_frame[] = { 0x01, 0x02, 0x03, 0x04 };
    can_communication_api_add_to_rx(CAN_COMM_NODE_2, dummy_frame, sizeof(dummy_frame));

    // Act: Pass the address of our mock state variable into the processing module
    enum CanCommunicationReturnCode rc = can_communication_api_process_rx(&mock_global_vehicle_state);

    // Assert
    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_OK, rc, "RX processing sweep failed to execute cleanly with valid pointer bindings");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, mock_can_deserialize_fake.call_count, "PAL processing loop failed to trigger downstream deserialization callback");

    // Verify the context pointer was safely delivered completely unmodified through the layers
    TEST_ASSERT_EQUAL_PTR_MESSAGE(&mock_global_vehicle_state, mock_can_deserialize_fake.arg1_val, "The state tracking pointer address was corrupted or misplaced during context routing");

    const struct PalMessage *received_msg = mock_can_deserialize_fake.arg0_val;
    TEST_ASSERT_NOT_NULL_MESSAGE(received_msg, "Passed message pointer to deserializer evaluates to NULL");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(sizeof(dummy_frame), received_msg->size, "Extracted payload size mismatch in RX callback");
}

void test_can_communication_api_process_rx_should_do_nothing_and_return_ok_when_queues_are_empty(void) {
    uint32_t dummy_state = 0;

    enum CanCommunicationReturnCode rc = can_communication_api_process_rx(&dummy_state);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_OK, rc, "Processing sweep failed or panicked when evaluating clean, empty queues");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, mock_can_deserialize_fake.call_count, "Downstream deserialization callback was erroneously triggered on an empty mailbox");
}

void test_can_communication_api_process_rx_should_drain_entire_queue_until_empty(void) {
    uint8_t dummy_payload[] = { 0xAA, 0xBB };
    uint32_t dummy_state = 0;
    uint32_t expected_messages = 3;

    for (uint32_t i = 0; i < expected_messages; i++) {
        (void)can_communication_api_add_to_rx(CAN_COMM_NODE_1, dummy_payload, sizeof(dummy_payload));
    }

    enum CanCommunicationReturnCode rc = can_communication_api_process_rx(&dummy_state);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_OK, rc, "Processing sweep failed while unspooling deep buffered elements");
    TEST_ASSERT_EQUAL_INT_MESSAGE(expected_messages, mock_can_deserialize_fake.call_count, "The processing sweep failed to fire the callback for every single element staged in the queue");
}
/*! \} */

/*!
 * \defgroup can_communication_api_add_to_tx Tests for can_communication_api_add_to_tx function
 * \{
 */

void test_can_communication_api_add_to_tx_should_fail_on_null_buffer(void) {
    enum CanCommunicationReturnCode rc = can_communication_api_add_to_tx(CAN_COMM_NODE_3, NULL, 4U);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_NULL_POINTER, rc, "Failed to trap NULL buffer pointer during TX staging");
}

void test_can_communication_api_add_to_tx_should_succeed(void) {
    uint8_t tx_frame[] = { 0xDE, 0xAD, 0xBE, 0xEF };

    enum CanCommunicationReturnCode rc = can_communication_api_add_to_tx(CAN_COMM_NODE_3, tx_frame, sizeof(tx_frame));

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_OK, rc, "Failed to successfully route valid outgoing CAN payload into PAL TX queue");
}

void test_can_communication_api_add_to_tx_should_fail_with_buffer_full(void) {
    uint8_t dummy_payload[] = { 0xCC, 0xDD };
    enum CanCommunicationReturnCode rc = CAN_COMM_RC_OK;

    for (uint32_t i = 0; i < TEST_TX_CAPACITY; i++) {
        rc = can_communication_api_add_to_tx(CAN_COMM_NODE_2, dummy_payload, sizeof(dummy_payload));
        TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_OK, rc, "TX queue rejected inserts up to nominal capacity limits");
    }

    rc = can_communication_api_add_to_tx(CAN_COMM_NODE_2, dummy_payload, sizeof(dummy_payload));
    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_BUFFER_FULL, rc, "Module failed to trap and return BUFFER_FULL on TX queue saturation");
}
/*! \} */

/*!
 * \defgroup can_communication_api_process_tx Tests for can_communication_api_process_tx function
 * \{
 */

void test_can_communication_api_process_tx_should_trigger_send_callback(void) {
    uint8_t tx_frame[] = { 0xFF, 0x00 };
    can_communication_api_add_to_tx(CAN_COMM_NODE_1, tx_frame, sizeof(tx_frame));

    enum CanCommunicationReturnCode rc = can_communication_api_process_tx();

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_OK, rc, "TX processing sweep failed to return OK on cleanly staged data");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, mock_can_send_fake.call_count, "PAL processing loop failed to trigger downstream transmission callback");

    const struct PalMessage *sent_msg = mock_can_send_fake.arg0_val;
    TEST_ASSERT_NOT_NULL_MESSAGE(sent_msg, "Passed message pointer to hardware transmitter evaluates to NULL");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(sizeof(tx_frame), sent_msg->size, "Extracted payload size mismatch in TX callback");
}

void test_can_communication_api_process_tx_should_do_nothing_and_return_ok_when_queues_are_empty(void) {
    enum CanCommunicationReturnCode rc = can_communication_api_process_tx();

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_OK, rc, "Processing sweep failed or panicked when executing transmission flushes on empty pipelines");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, mock_can_send_fake.call_count, "Hardware transmitter callback fired on a dry queue with no staged frames");
}

void test_can_communication_api_process_tx_should_drain_entire_queue_until_empty(void) {
    uint8_t dummy_payload[] = { 0x11, 0x22 };
    uint32_t expected_messages = 5;

    for (uint32_t i = 0; i < expected_messages; i++) {
        (void)can_communication_api_add_to_tx(CAN_COMM_NODE_3, dummy_payload, sizeof(dummy_payload));
    }

    enum CanCommunicationReturnCode rc = can_communication_api_process_tx();

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_OK, rc, "TX flush sweep failed while processing deep buffered transmission pipelines");
    TEST_ASSERT_EQUAL_INT_MESSAGE(expected_messages, mock_can_send_fake.call_count, "The processing sweep failed to transmit every single element staged in the queue");
}
/*! \} */

int main(void) {
    UNITY_BEGIN();

    /*!
     * \addtogroup can_communication_api_init
     * \{
     */
    RUN_TEST(test_can_communication_api_init_should_fail_on_null_pointer);
    RUN_TEST(test_can_communication_api_init_should_succeed_with_correct_pointers);
    /*! \} */

    /*!
     * \addtogroup can_communication_api_add_to_rx
     * \{
     */
    RUN_TEST(test_can_communication_api_add_to_rx_should_fail_on_invalid_node);
    RUN_TEST(test_can_communication_api_add_to_rx_should_succeed);
    RUN_TEST(test_can_communication_api_add_to_rx_should_fail_with_buffer_full);
    /*! \} */

    /*!
     * \addtogroup can_communication_api_process_rx
     * \{
     */
    RUN_TEST(test_can_communication_api_process_rx_should_fail_if_uninitialized);
    RUN_TEST(test_can_communication_api_process_rx_should_trigger_deserialize_callback_with_provided_state);
    RUN_TEST(test_can_communication_api_process_rx_should_do_nothing_and_return_ok_when_queues_are_empty);
    RUN_TEST(test_can_communication_api_process_rx_should_drain_entire_queue_until_empty);
    /*! \} */

    /*!
     * \addtogroup can_communication_api_add_to_tx
     * \{
     */
    RUN_TEST(test_can_communication_api_add_to_tx_should_fail_on_null_buffer);
    RUN_TEST(test_can_communication_api_add_to_tx_should_succeed);
    RUN_TEST(test_can_communication_api_add_to_tx_should_fail_with_buffer_full);
    /*! \} */

    /*!
     * \addtogroup can_communication_api_process_tx
     * \{
     */
    RUN_TEST(test_can_communication_api_process_tx_should_trigger_send_callback);
    RUN_TEST(test_can_communication_api_process_tx_should_do_nothing_and_return_ok_when_queues_are_empty);
    RUN_TEST(test_can_communication_api_process_tx_should_drain_entire_queue_until_empty);
    /*! \} */

    return UNITY_END();
}