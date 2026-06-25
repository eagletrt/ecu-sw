/*!
 * \file test_can_communication.c
 * \author Dorijan Di Zepp
 * \date 2026-06-24
 * \brief Unit tests using FFF and Unity for testing the CAN communication module.
 */

#include "unity.h"
#include "fff.h"
#include "can-communication-api.h"
#include "eagletrt-api.h"
#include <string.h>

DEFINE_FFF_GLOBALS;

#define TEST_TRACKING_BUFFER_SIZE (5U)

extern struct CanCommunicationHandler handler;

EAGLETRT_STATIC struct CanCommunicationFrame test_captured_rx_frames[TEST_TRACKING_BUFFER_SIZE];
EAGLETRT_STATIC uint32_t test_captured_rx_count = 0U;

FAKE_VALUE_FUNC(enum CanCommunicationReturnCode, mock_can_send, const struct CanCommunicationFrame *);
FAKE_VALUE_FUNC(enum CanCommunicationReturnCode, mock_application_on_receive, const struct CanCommunicationFrame *);

/* Configurations used to automatically initialize the module in setUp */
EAGLETRT_STATIC struct CanCommunicationNetworkConfig test_valid_configs[CAN_COMMUNICATION_NET_COUNT];

EAGLETRT_STATIC enum CanCommunicationReturnCode prv_custom_on_receive_capture(const struct CanCommunicationFrame *frame) {
    if ((frame != NULL) && (test_captured_rx_count < TEST_TRACKING_BUFFER_SIZE)) {
        test_captured_rx_frames[test_captured_rx_count] = *frame;
        test_captured_rx_count++;
    }
    return CAN_COMMUNICATION_RC_OK;
}

EAGLETRT_STATIC enum CanCommunicationReturnCode prv_custom_on_receive_failing(const struct CanCommunicationFrame *frame) {
    EAGLETRT_API_UNUSED(frame);
    return CAN_COMMUNICATION_RC_ERROR;
}

void setUp(void) {
    RESET_FAKE(mock_can_send);
    RESET_FAKE(mock_application_on_receive);
    FFF_RESET_HISTORY();

    mock_can_send_fake.return_val = CAN_COMMUNICATION_RC_OK;
    mock_application_on_receive_fake.return_val = CAN_COMMUNICATION_RC_OK;
    mock_can_send_fake.custom_fake = NULL;
    mock_application_on_receive_fake.custom_fake = NULL;

    memset(&handler, 0, sizeof(handler));
    memset(test_captured_rx_frames, 0, sizeof(test_captured_rx_frames));
    test_captured_rx_count = 0U;

    // Prepare the configuration
    for (size_t i = 0; i < CAN_COMMUNICATION_NET_COUNT; ++i) {
        test_valid_configs[i].send = mock_can_send;
        test_valid_configs[i].on_receive = mock_application_on_receive;
        test_valid_configs[i].cs_enter = NULL;
        test_valid_configs[i].cs_exit = NULL;
    }

    can_communication_api_init(test_valid_configs);
}

/* --- Test Cases --- */

/*!
 * \defgroup can_communication_api_init Tests for can_communication_api_init
 * \{
 */

void test_can_communication_api_init_should_fail_on_null_config(void) {
    memset(&handler, 0, sizeof(handler));

    enum CanCommunicationReturnCode rc = can_communication_api_init(NULL);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMMUNICATION_RC_NULL_POINTER, rc, "Module initialization did not fail when passed a completely NULL configuration array");
}

void test_can_communication_api_init_should_fail_on_missing_mandatory_send_callback(void) {
    memset(&handler, 0, sizeof(handler));

    struct CanCommunicationNetworkConfig invalid_configs[CAN_COMMUNICATION_NET_COUNT] = {
        [CAN_COMMUNICATION_NET_PRIMARY] = { .send = NULL, .on_receive = mock_application_on_receive },
        [CAN_COMMUNICATION_NET_SECONDARY] = { .send = mock_can_send, .on_receive = mock_application_on_receive },
        [CAN_COMMUNICATION_NET_INVERTER] = { .send = mock_can_send, .on_receive = mock_application_on_receive }
    };

    enum CanCommunicationReturnCode rc = can_communication_api_init(invalid_configs);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMMUNICATION_RC_NULL_POINTER, rc, "Module initialization bypassed safety checks when mandatory driver send callback was unassigned");
}

void test_can_communication_api_init_should_fail_on_missing_mandatory_receive_callback(void) {
    memset(&handler, 0, sizeof(handler));

    struct CanCommunicationNetworkConfig invalid_configs[CAN_COMMUNICATION_NET_COUNT] = {
        [CAN_COMMUNICATION_NET_PRIMARY] = { .send = mock_can_send, .on_receive = mock_application_on_receive },
        [CAN_COMMUNICATION_NET_SECONDARY] = { .send = mock_can_send, .on_receive = NULL },
        [CAN_COMMUNICATION_NET_INVERTER] = { .send = mock_can_send, .on_receive = mock_application_on_receive }
    };

    enum CanCommunicationReturnCode rc = can_communication_api_init(invalid_configs);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMMUNICATION_RC_NULL_POINTER, rc, "Module initialization bypassed safety checks when mandatory application receive callback was unassigned");
}

void test_can_communication_api_init_should_succeed_with_correct_parameters(void) {
    memset(&handler, 0, sizeof(handler));

    enum CanCommunicationReturnCode rc = can_communication_api_init(test_valid_configs);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMMUNICATION_RC_OK, rc, "Module initialization reported an internal initialization error under nominal, valid parameters");
}
/*! \} */

/*!
 * \defgroup can_communication_api_add_to_rx Tests for can_communication_api_add_to_rx
 * \{
 */

void test_can_communication_api_add_to_rx_should_fail_on_null_frame(void) {
    enum CanCommunicationReturnCode rc = can_communication_api_add_to_rx(CAN_COMMUNICATION_NET_PRIMARY, NULL);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMMUNICATION_RC_NULL_POINTER, rc, "Module allowed a NULL frame pointer reference to be queued inside the RX pipeline");
}

void test_can_communication_api_add_to_rx_should_fail_on_invalid_length(void) {
    struct CanCommunicationFrame frame = { .id = 0x100U, .length = 9U, .data = { 0 } };

    enum CanCommunicationReturnCode rc = can_communication_api_add_to_rx(CAN_COMMUNICATION_NET_PRIMARY, &frame);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMMUNICATION_RC_INVALID_LENGTH, rc, "Module failed to protect ring buffer boundaries when passed a payload length exceeding Classic CAN definitions");
}

void test_can_communication_api_add_to_rx_should_fail_on_invalid_network_bounds(void) {
    struct CanCommunicationFrame frame = { .id = 0x100U, .length = 4U, .data = { 0 } };

    enum CanCommunicationReturnCode rc = can_communication_api_add_to_rx(CAN_COMMUNICATION_NET_COUNT, &frame);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMMUNICATION_RC_INVALID_NETWORK, rc, "Module accepted an insertion onto a completely out-of-bounds network segment parameter index");
}

void test_can_communication_api_add_to_rx_should_fail_with_queue_full(void) {
    enum CanCommunicationReturnCode nominal_rcs[CAN_COMMUNICATION_RX_QUEUE_CAPACITY];
    enum CanCommunicationReturnCode saturation_rc;
    struct CanCommunicationFrame frame = { .id = 0x123U, .length = 8U, .data = { 1, 2, 3, 4, 5, 6, 7, 8 } };

    for (uint32_t i = 0; i < CAN_COMMUNICATION_RX_QUEUE_CAPACITY; i++) {
        nominal_rcs[i] = can_communication_api_add_to_rx(CAN_COMMUNICATION_NET_PRIMARY, &frame);
    }

    saturation_rc = can_communication_api_add_to_rx(CAN_COMMUNICATION_NET_PRIMARY, &frame);

    TEST_ASSERT_EACH_EQUAL_INT_MESSAGE(CAN_COMMUNICATION_RC_OK, nominal_rcs, CAN_COMMUNICATION_RX_QUEUE_CAPACITY, "Queue insertion failed unexpectedly prior to hitting nominal tracking capacity milestones");
    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMMUNICATION_RC_QUEUE_FULL, saturation_rc, "Module failed to return a saturation code error after memory layout tracks reached complete capacity limits");
}
/*! \} */

/*!
 * \defgroup can_communication_api_process_rx Tests for can_communication_api_process_rx
 * \{
 */

void test_can_communication_api_process_rx_should_fail_on_invalid_network_bounds(void) {
    enum CanCommunicationReturnCode rc = can_communication_api_process_rx(CAN_COMMUNICATION_NET_COUNT);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMMUNICATION_RC_INVALID_NETWORK, rc, "Processing cycle bypass bounds verification rules when passed an out-of-bounds network context index");
}

void test_can_communication_api_process_rx_should_drain_entire_queue_to_dispatcher(void) {
    enum CanCommunicationReturnCode process_rc;
    struct CanCommunicationFrame raw_frame_one = { .id = 0x201U, .length = 2U, .data = { 0xAA, 0xBB } };
    struct CanCommunicationFrame raw_frame_two = { .id = 0x302U, .length = 4U, .data = { 0x11, 0x22, 0x33, 0x44 } };

    mock_application_on_receive_fake.custom_fake = prv_custom_on_receive_capture;
    can_communication_api_add_to_rx(CAN_COMMUNICATION_NET_PRIMARY, &raw_frame_one);
    can_communication_api_add_to_rx(CAN_COMMUNICATION_NET_PRIMARY, &raw_frame_two);

    process_rc = can_communication_api_process_rx(CAN_COMMUNICATION_NET_PRIMARY);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMMUNICATION_RC_OK, process_rc, "The processing execution routine returned an error status while draining valid incoming frames");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(2U, test_captured_rx_count, "The processing sweep did not dispatch all items currently queued within the target reception line");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0x201U, test_captured_rx_frames[0].id, "First dispatched message ID tracking corrupted or sequence order misplaced during stack translation");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2U, test_captured_rx_frames[0].length, "First payload size definition corrupted or mismatched during parsing operations");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0x302U, test_captured_rx_frames[1].id, "Second dispatched message ID tracking corrupted or sequence order misplaced during stack translation");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(4U, test_captured_rx_frames[1].length, "Second payload size definition corrupted or mismatched during parsing operations");
}

void test_can_communication_api_process_rx_should_forward_receive_handler_errors(void) {
    enum CanCommunicationReturnCode process_rc;
    struct CanCommunicationFrame raw_frame = { .id = 0x201U, .length = 2U, .data = { 0xAA, 0xBB } };

    mock_application_on_receive_fake.custom_fake = prv_custom_on_receive_failing;
    can_communication_api_add_to_rx(CAN_COMMUNICATION_NET_PRIMARY, &raw_frame);

    process_rc = can_communication_api_process_rx(CAN_COMMUNICATION_NET_PRIMARY);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMMUNICATION_RC_RECEIVE_HANDLER_ERROR, process_rc, "The processing loop did not capture or bubble up unexpected handler failures occurring within application routines");
}
/*! \} */

/*!
 * \defgroup can_communication_api_add_to_tx Tests for can_communication_api_add_to_tx
 * \{
 */

void test_can_communication_api_add_to_tx_should_fail_on_null_frame(void) {
    enum CanCommunicationReturnCode rc = can_communication_api_add_to_tx(CAN_COMMUNICATION_NET_INVERTER, NULL);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMMUNICATION_RC_NULL_POINTER, rc, "Staging an outgoing frame failed to intercept a dangerous NULL data pointer sequence entry");
}

void test_can_communication_api_add_to_tx_should_fail_on_invalid_length(void) {
    struct CanCommunicationFrame frame = { .id = 0x50U, .length = 9U, .data = { 0 } };

    enum CanCommunicationReturnCode rc = can_communication_api_add_to_tx(CAN_COMMUNICATION_NET_INVERTER, &frame);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMMUNICATION_RC_INVALID_LENGTH, rc, "Transmission tracking bypassed Classic CAN layout frame length validation parameters");
}

void test_can_communication_api_add_to_tx_should_fail_on_invalid_network_bounds(void) {
    struct CanCommunicationFrame frame = { .id = 0x50U, .length = 1U, .data = { 0xFF } };

    enum CanCommunicationReturnCode rc = can_communication_api_add_to_tx(CAN_COMMUNICATION_NET_COUNT, &frame);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMMUNICATION_RC_INVALID_NETWORK, rc, "Transmission tracking line failed to intercept an out-of-bounds network identity selection");
}

void test_can_communication_api_add_to_tx_should_succeed(void) {
    struct CanCommunicationFrame frame = { .id = 0x50U, .length = 1U, .data = { 0xFF } };

    enum CanCommunicationReturnCode rc = can_communication_api_add_to_tx(CAN_COMMUNICATION_NET_INVERTER, &frame);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMMUNICATION_RC_OK, rc, "Staging an outgoing abstract frame structure into transmission lines reported unexpected operation exceptions");
}

void test_can_communication_api_add_to_tx_should_fail_with_queue_full(void) {
    enum CanCommunicationReturnCode nominal_rcs[CAN_COMMUNICATION_TX_QUEUE_CAPACITY];
    enum CanCommunicationReturnCode saturation_rc;
    struct CanCommunicationFrame frame = { .id = 0x50U, .length = 1U, .data = { 0xFF } };

    for (uint32_t i = 0; i < CAN_COMMUNICATION_TX_QUEUE_CAPACITY; i++) {
        nominal_rcs[i] = can_communication_api_add_to_tx(CAN_COMMUNICATION_NET_INVERTER, &frame);
    }

    saturation_rc = can_communication_api_add_to_tx(CAN_COMMUNICATION_NET_INVERTER, &frame);

    TEST_ASSERT_EACH_EQUAL_INT_MESSAGE(CAN_COMMUNICATION_RC_OK, nominal_rcs, CAN_COMMUNICATION_TX_QUEUE_CAPACITY, "Transmission queue ring rejected allocation insertions before hitting true limits");
    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMMUNICATION_RC_QUEUE_FULL, saturation_rc, "Transmission pipeline failed to report queue saturation code configurations upon limits overflow");
}
/*! \} */

/*!
 * \defgroup can_communication_api_process_tx Tests for can_communication_api_process_tx
 * \{
 */

void test_can_communication_api_process_tx_should_fail_on_invalid_network_bounds(void) {
    enum CanCommunicationReturnCode rc = can_communication_api_process_tx(CAN_COMMUNICATION_NET_COUNT);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMMUNICATION_RC_INVALID_NETWORK, rc, "Transmission pipeline processing accepted out-of-bounds context validation configurations");
}

void test_can_communication_api_process_tx_should_flush_to_hardware(void) {
    enum CanCommunicationReturnCode process_rc;
    struct CanCommunicationFrame frame = { .id = 0x77U, .length = 4U, .data = { 0x1, 0x2, 0x3, 0x4 } };

    can_communication_api_add_to_tx(CAN_COMMUNICATION_NET_INVERTER, &frame);

    process_rc = can_communication_api_process_tx(CAN_COMMUNICATION_NET_INVERTER);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMMUNICATION_RC_OK, process_rc, "The transmission processing loop panicked or reported failure status during execution flushes");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, mock_can_send_fake.call_count, "The module failed to invoke your registered hardware transmitter handler hook exactly once per frame block");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0x77U, mock_can_send_fake.arg0_val->id, "The hardware data package packet structure was passed containing corrupted or modified address values");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(4U, mock_can_send_fake.arg0_val->length, "The data payload array window boundaries passed to the peripheral driver were modified or corrupted");
}

void test_can_communication_api_process_tx_should_forward_hardware_io_errors(void) {
    enum CanCommunicationReturnCode process_rc;
    struct CanCommunicationFrame frame = { .id = 0x77U, .length = 4U, .data = { 0x1, 0x2, 0x3, 0x4 } };

    mock_can_send_fake.return_val = CAN_COMMUNICATION_RC_ERROR;
    can_communication_api_add_to_tx(CAN_COMMUNICATION_NET_INVERTER, &frame);

    process_rc = can_communication_api_process_tx(CAN_COMMUNICATION_NET_INVERTER);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMMUNICATION_RC_TRANSMISSION_ERROR, process_rc, "Transmission module processing failed to turn peripheral IO errors into explicit transmission failure codes");
}
/*! \} */

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_can_communication_api_init_should_fail_on_null_config);
    RUN_TEST(test_can_communication_api_init_should_fail_on_missing_mandatory_send_callback);
    RUN_TEST(test_can_communication_api_init_should_fail_on_missing_mandatory_receive_callback);
    RUN_TEST(test_can_communication_api_init_should_succeed_with_correct_parameters);

    RUN_TEST(test_can_communication_api_add_to_rx_should_fail_on_null_frame);
    RUN_TEST(test_can_communication_api_add_to_rx_should_fail_on_invalid_length);
    RUN_TEST(test_can_communication_api_add_to_rx_should_fail_on_invalid_network_bounds);
    RUN_TEST(test_can_communication_api_add_to_rx_should_fail_with_queue_full);

    RUN_TEST(test_can_communication_api_process_rx_should_fail_on_invalid_network_bounds);
    RUN_TEST(test_can_communication_api_process_rx_should_drain_entire_queue_to_dispatcher);
    RUN_TEST(test_can_communication_api_process_rx_should_forward_receive_handler_errors);

    RUN_TEST(test_can_communication_api_add_to_tx_should_fail_on_null_frame);
    RUN_TEST(test_can_communication_api_add_to_tx_should_fail_on_invalid_length);
    RUN_TEST(test_can_communication_api_add_to_tx_should_fail_on_invalid_network_bounds);
    RUN_TEST(test_can_communication_api_add_to_tx_should_succeed);
    RUN_TEST(test_can_communication_api_add_to_tx_should_fail_with_queue_full);

    RUN_TEST(test_can_communication_api_process_tx_should_fail_on_invalid_network_bounds);
    RUN_TEST(test_can_communication_api_process_tx_should_flush_to_hardware);
    RUN_TEST(test_can_communication_api_process_tx_should_forward_hardware_io_errors);

    return UNITY_END();
}