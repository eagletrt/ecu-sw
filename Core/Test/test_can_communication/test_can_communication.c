/*!
 * \file test_can_communication_api.c
 * \author Dorijan Di Zepp
 * \date 2026-06-20
 * \brief Unit tests using FFF and Unity for testing the CAN communication module.
 */

#include "unity.h"
#include "fff.h"
#include "can-communication-api.h"
#include "eagletrt-api.h"
#include <string.h>

DEFINE_FFF_GLOBALS;

#define TEST_TRACKING_BUFFER_SIZE (5U)

extern struct CanCommunicationHandler can_comm_handler;

EAGLETRT_STATIC struct CanCommunicationFrame test_captured_rx_frames[TEST_TRACKING_BUFFER_SIZE];
EAGLETRT_STATIC uint32_t test_captured_rx_count = 0U;

FAKE_VALUE_FUNC(enum CanCommunicationReturnCode, mock_can_send, const struct CanCommunicationFrame *);
FAKE_VALUE_FUNC(enum CanCommunicationReturnCode, mock_application_on_receive, const struct CanCommunicationFrame *);

EAGLETRT_STATIC enum CanCommunicationReturnCode prv_custom_on_receive_capture(const struct CanCommunicationFrame *frame) {
    if ((frame != NULL) && (test_captured_rx_count < TEST_TRACKING_BUFFER_SIZE)) {
        test_captured_rx_frames[test_captured_rx_count] = *frame;
        test_captured_rx_count++;
    }
    return CAN_COMM_RC_OK;
}

EAGLETRT_STATIC enum CanCommunicationReturnCode prv_custom_on_receive_failing(const struct CanCommunicationFrame *frame) {
    EAGLETRT_API_UNUSED(frame);
    return CAN_COMM_RC_ERROR;
}

void setUp(void) {
    RESET_FAKE(mock_can_send);
    RESET_FAKE(mock_application_on_receive);
    FFF_RESET_HISTORY();

    mock_can_send_fake.return_val = CAN_COMM_RC_OK;
    mock_application_on_receive_fake.return_val = CAN_COMM_RC_OK;
    mock_can_send_fake.custom_fake = NULL;
    mock_application_on_receive_fake.custom_fake = NULL;

    memset(&can_comm_handler, 0, sizeof(can_comm_handler));
    memset(test_captured_rx_frames, 0, sizeof(test_captured_rx_frames));
    test_captured_rx_count = 0U;
}

/* =========================================================================
 * TEST CASES
 * ========================================================================= */

/*!
 * \defgroup can_communication_api_init Tests for can_communication_api_init
 * \{
 */

void test_can_communication_api_init_should_fail_on_null_config(void) {
    enum CanCommunicationReturnCode rc = can_communication_api_init(CAN_COMM_NET_PRIMARY, NULL);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_NULL_POINTER, rc, "Module initialization did not fail when passed a completely NULL configuration pointer block");
}

void test_can_communication_api_init_should_fail_on_missing_mandatory_send_callback(void) {
    struct CanCommunicationNetworkConfig invalid_config = {
        .send = NULL,
        .on_receive = mock_application_on_receive,
        .cs_enter = NULL,
        .cs_exit = NULL
    };

    enum CanCommunicationReturnCode rc = can_communication_api_init(CAN_COMM_NET_PRIMARY, &invalid_config);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_NULL_POINTER, rc, "Module initialization bypassed safety checks when mandatory hardware driver send callback was unassigned");
}

void test_can_communication_api_init_should_fail_on_missing_mandatory_receive_callback(void) {
    struct CanCommunicationNetworkConfig invalid_config = {
        .send = mock_can_send,
        .on_receive = NULL,
        .cs_enter = NULL,
        .cs_exit = NULL
    };

    enum CanCommunicationReturnCode rc = can_communication_api_init(CAN_COMM_NET_PRIMARY, &invalid_config);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_NULL_POINTER, rc, "Module initialization bypassed safety checks when mandatory application receive callback was unassigned");
}

void test_can_communication_api_init_should_fail_on_invalid_network_bounds(void) {
    struct CanCommunicationNetworkConfig config = {
        .send = mock_can_send,
        .on_receive = mock_application_on_receive,
        .cs_enter = NULL,
        .cs_exit = NULL
    };

    enum CanCommunicationReturnCode rc = can_communication_api_init(CAN_COMM_NET_COUNT, &config);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_INVALID_ARGUMENT, rc, "Module boundary check failed to block an out-of-bounds network ID sentinel value during layout setup");
}

void test_can_communication_api_init_should_succeed_with_correct_parameters(void) {
    struct CanCommunicationNetworkConfig config = {
        .send = mock_can_send,
        .on_receive = mock_application_on_receive,
        .cs_enter = NULL,
        .cs_exit = NULL
    };

    enum CanCommunicationReturnCode rc = can_communication_api_init(CAN_COMM_NET_PRIMARY, &config);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_OK, rc, "Module initialization reported an internal initialization error under nominal, valid parameters");
}
/*! \} */

/*!
 * \defgroup can_communication_api_add_to_rx Tests for can_communication_api_add_to_rx
 * \{
 */

void test_can_communication_api_add_to_rx_should_fail_if_uninitialized(void) {
    struct CanCommunicationFrame frame = { .id = 0x100U, .length = 4U, .data = { 0 } };

    enum CanCommunicationReturnCode rc = can_communication_api_add_to_rx(CAN_COMM_NET_SECONDARY, &frame);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_NOT_INITIALIZED, rc, "Module allowed an external data write transaction to process onto an unconfigured network track segment");
}

void test_can_communication_api_add_to_rx_should_fail_on_null_frame(void) {
    struct CanCommunicationNetworkConfig config = { .send = mock_can_send, .on_receive = mock_application_on_receive };
    (void)can_communication_api_init(CAN_COMM_NET_PRIMARY, &config);

    enum CanCommunicationReturnCode rc = can_communication_api_add_to_rx(CAN_COMM_NET_PRIMARY, NULL);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_NULL_POINTER, rc, "Module allowed a NULL frame pointer reference to be queued inside the RX pipeline");
}

void test_can_communication_api_add_to_rx_should_fail_on_invalid_length(void) {
    struct CanCommunicationNetworkConfig config = { .send = mock_can_send, .on_receive = mock_application_on_receive };
    struct CanCommunicationFrame frame = { .id = 0x100U, .length = 9U, .data = { 0 } };
    (void)can_communication_api_init(CAN_COMM_NET_PRIMARY, &config);

    enum CanCommunicationReturnCode rc = can_communication_api_add_to_rx(CAN_COMM_NET_PRIMARY, &frame);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_INVALID_LENGTH, rc, "Module failed to protect ring buffer boundaries when passed a payload length exceeding Classic CAN definitions");
}

void test_can_communication_api_add_to_rx_should_fail_on_invalid_network_bounds(void) {
    struct CanCommunicationFrame frame = { .id = 0x100U, .length = 4U, .data = { 0 } };

    enum CanCommunicationReturnCode rc = can_communication_api_add_to_rx(CAN_COMM_NET_COUNT, &frame);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_INVALID_ARGUMENT, rc, "Module accepted an insertion onto a completely out-of-bounds network segment parameter index");
}

void test_can_communication_api_add_to_rx_should_fail_with_buffer_full(void) {
    enum CanCommunicationReturnCode nominal_rcs[CAN_COMMUNICATION_RX_QUEUE_CAPACITY];
    enum CanCommunicationReturnCode saturation_rc;
    struct CanCommunicationNetworkConfig config = { .send = mock_can_send, .on_receive = mock_application_on_receive };
    struct CanCommunicationFrame frame = { .id = 0x123U, .length = 8U, .data = { 1, 2, 3, 4, 5, 6, 7, 8 } };

    can_communication_api_init(CAN_COMM_NET_PRIMARY, &config);

    for (uint32_t i = 0; i < CAN_COMMUNICATION_RX_QUEUE_CAPACITY; i++) {
        nominal_rcs[i] = can_communication_api_add_to_rx(CAN_COMM_NET_PRIMARY, &frame);
    }

    saturation_rc = can_communication_api_add_to_rx(CAN_COMM_NET_PRIMARY, &frame);

    TEST_ASSERT_EACH_EQUAL_INT_MESSAGE(CAN_COMM_RC_OK, nominal_rcs, CAN_COMMUNICATION_RX_QUEUE_CAPACITY, "Queue insertion failed unexpectedly prior to hitting nominal tracking capacity milestones");
    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_BUFFER_FULL, saturation_rc, "Module failed to return a saturation code error after memory layout tracks reached complete capacity limits");
}
/*! \} */

/*!
 * \defgroup can_communication_api_process_rx Tests for can_communication_api_process_rx
 * \{
 */

void test_can_communication_api_process_rx_should_fail_if_uninitialized(void) {
    enum CanCommunicationReturnCode rc = can_communication_api_process_rx(CAN_COMM_NET_PRIMARY);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_NOT_INITIALIZED, rc, "Processing execution triggered execution sweeps across an uninitialized track module channel");
}

void test_can_communication_api_process_rx_should_fail_on_invalid_network_bounds(void) {
    enum CanCommunicationReturnCode rc = can_communication_api_process_rx(CAN_COMM_NET_COUNT);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_INVALID_ARGUMENT, rc, "Processing cycle bypass bounds verification rules when passed an out-of-bounds network context index");
}

void test_can_communication_api_process_rx_should_drain_entire_queue_to_dispatcher(void) {
    enum CanCommunicationReturnCode process_rc;
    struct CanCommunicationNetworkConfig config = { .send = mock_can_send, .on_receive = mock_application_on_receive };
    struct CanCommunicationFrame raw_frame_one = { .id = 0x201U, .length = 2U, .data = { 0xAA, 0xBB } };
    struct CanCommunicationFrame raw_frame_two = { .id = 0x302U, .length = 4U, .data = { 0x11, 0x22, 0x33, 0x44 } };

    mock_application_on_receive_fake.custom_fake = prv_custom_on_receive_capture;
    can_communication_api_init(CAN_COMM_NET_PRIMARY, &config);
    can_communication_api_add_to_rx(CAN_COMM_NET_PRIMARY, &raw_frame_one);
    can_communication_api_add_to_rx(CAN_COMM_NET_PRIMARY, &raw_frame_two);

    process_rc = can_communication_api_process_rx(CAN_COMM_NET_PRIMARY);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_OK, process_rc, "The processing execution routine returned an error status while draining valid incoming frames");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(2U, test_captured_rx_count, "The processing sweep did not dispatch all items currently queued within the target reception line");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0x201U, test_captured_rx_frames[0].id, "First dispatched message ID tracking corrupted or sequence order misplaced during stack translation");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2U, test_captured_rx_frames[0].length, "First payload size definition corrupted or mismatched during parsing operations");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0x302U, test_captured_rx_frames[1].id, "Second dispatched message ID tracking corrupted or sequence order misplaced during stack translation");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(4U, test_captured_rx_frames[1].length, "Second payload size definition corrupted or mismatched during parsing operations");
}

void test_can_communication_api_process_rx_should_forward_receive_handler_errors(void) {
    enum CanCommunicationReturnCode process_rc;
    struct CanCommunicationNetworkConfig config = { .send = mock_can_send, .on_receive = mock_application_on_receive };
    struct CanCommunicationFrame raw_frame = { .id = 0x201U, .length = 2U, .data = { 0xAA, 0xBB } };

    mock_application_on_receive_fake.custom_fake = prv_custom_on_receive_failing;
    can_communication_api_init(CAN_COMM_NET_PRIMARY, &config);
    can_communication_api_add_to_rx(CAN_COMM_NET_PRIMARY, &raw_frame);

    process_rc = can_communication_api_process_rx(CAN_COMM_NET_PRIMARY);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_RECEIVE_HANDLER_ERROR, process_rc, "The processing loop did not capture or bubbled up unexpected handler failures occurring within application routines");
}
/*! \} */

/*!
 * \defgroup can_communication_api_add_to_tx Tests for can_communication_api_add_to_tx
 * \{
 */

void test_can_communication_api_add_to_tx_should_fail_if_uninitialized(void) {
    struct CanCommunicationFrame frame = { .id = 0x50U, .length = 1U, .data = { 0xFF } };

    enum CanCommunicationReturnCode rc = can_communication_api_add_to_tx(CAN_COMM_NET_INVERTER, &frame);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_NOT_INITIALIZED, rc, "Module allowed data writing transaction to process onto an unconfigured transmission line segment");
}

void test_can_communication_api_add_to_tx_should_fail_on_null_frame(void) {
    struct CanCommunicationNetworkConfig config = { .send = mock_can_send, .on_receive = mock_application_on_receive };
    can_communication_api_init(CAN_COMM_NET_INVERTER, &config);

    enum CanCommunicationReturnCode rc = can_communication_api_add_to_tx(CAN_COMM_NET_INVERTER, NULL);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_NULL_POINTER, rc, "Staging an outgoing frame failed to intercept a dangerous NULL data pointer sequence entry");
}

void test_can_communication_api_add_to_tx_should_fail_on_invalid_length(void) {
    struct CanCommunicationNetworkConfig config = { .send = mock_can_send, .on_receive = mock_application_on_receive };
    struct CanCommunicationFrame frame = { .id = 0x50U, .length = 9U, .data = { 0 } };
    can_communication_api_init(CAN_COMM_NET_INVERTER, &config);

    enum CanCommunicationReturnCode rc = can_communication_api_add_to_tx(CAN_COMM_NET_INVERTER, &frame);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_INVALID_LENGTH, rc, "Transmission tracking bypassed Classic CAN layout frame length validation parameters");
}

void test_can_communication_api_add_to_tx_should_fail_on_invalid_network_bounds(void) {
    struct CanCommunicationFrame frame = { .id = 0x50U, .length = 1U, .data = { 0xFF } };

    enum CanCommunicationReturnCode rc = can_communication_api_add_to_tx(CAN_COMM_NET_COUNT, &frame);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_INVALID_ARGUMENT, rc, "Transmission tracking line failed to intercept an out-of-bounds network identity selection");
}

void test_can_communication_api_add_to_tx_should_succeed(void) {
    struct CanCommunicationNetworkConfig config = { .send = mock_can_send, .on_receive = mock_application_on_receive };
    struct CanCommunicationFrame frame = { .id = 0x50U, .length = 1U, .data = { 0xFF } };
    can_communication_api_init(CAN_COMM_NET_INVERTER, &config);

    enum CanCommunicationReturnCode rc = can_communication_api_add_to_tx(CAN_COMM_NET_INVERTER, &frame);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_OK, rc, "Staging an outgoing abstract frame structure into transmission lines reported unexpected operation exceptions");
}

void test_can_communication_api_add_to_tx_should_fail_with_buffer_full(void) {
    enum CanCommunicationReturnCode nominal_rcs[CAN_COMMUNICATION_TX_QUEUE_CAPACITY];
    enum CanCommunicationReturnCode saturation_rc;
    struct CanCommunicationNetworkConfig config = { .send = mock_can_send, .on_receive = mock_application_on_receive };
    struct CanCommunicationFrame frame = { .id = 0x50U, .length = 1U, .data = { 0xFF } };

    can_communication_api_init(CAN_COMM_NET_INVERTER, &config);

    for (uint32_t i = 0; i < CAN_COMMUNICATION_TX_QUEUE_CAPACITY; i++) {
        nominal_rcs[i] = can_communication_api_add_to_tx(CAN_COMM_NET_INVERTER, &frame);
    }

    saturation_rc = can_communication_api_add_to_tx(CAN_COMM_NET_INVERTER, &frame);

    TEST_ASSERT_EACH_EQUAL_INT_MESSAGE(CAN_COMM_RC_OK, nominal_rcs, CAN_COMMUNICATION_TX_QUEUE_CAPACITY, "Transmission queue ring rejected allocation insertions before hitting true limits");
    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_BUFFER_FULL, saturation_rc, "Transmission pipeline failed to report queue saturation code configurations upon limits overflow");
}
/*! \} */

/*!
 * \defgroup can_communication_api_process_tx Tests for can_communication_api_process_tx
 * \{
 */

void test_can_communication_api_process_tx_should_fail_if_uninitialized(void) {
    enum CanCommunicationReturnCode rc = can_communication_api_process_tx(CAN_COMM_NET_INVERTER);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_NOT_INITIALIZED, rc, "Transmission execution processing unspooled operational transactions against an unconfigured pipeline");
}

void test_can_communication_api_process_tx_should_fail_on_invalid_network_bounds(void) {
    enum CanCommunicationReturnCode rc = can_communication_api_process_tx(CAN_COMM_NET_COUNT);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_INVALID_ARGUMENT, rc, "Transmission pipeline processing accepted out-of-bounds context validation configurations");
}

void test_can_communication_api_process_tx_should_flush_to_hardware(void) {
    enum CanCommunicationReturnCode process_rc;
    struct CanCommunicationNetworkConfig config = { .send = mock_can_send, .on_receive = mock_application_on_receive };
    struct CanCommunicationFrame frame = { .id = 0x77U, .length = 4U, .data = { 0x1, 0x2, 0x3, 0x4 } };

    can_communication_api_init(CAN_COMM_NET_INVERTER, &config);
    can_communication_api_add_to_tx(CAN_COMM_NET_INVERTER, &frame);

    process_rc = can_communication_api_process_tx(CAN_COMM_NET_INVERTER);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_OK, process_rc, "The transmission processing loop panicked or reported failure status during execution flushes");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, mock_can_send_fake.call_count, "The module failed to invoke your registered hardware transmitter handler hook exactly once per frame block");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0x77U, mock_can_send_fake.arg0_val->id, "The hardware data package packet structure was passed containing corrupted or modified address values");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(4U, mock_can_send_fake.arg0_val->length, "The data payload array window boundaries passed to the peripheral driver were modified or corrupted");
}

void test_can_communication_api_process_tx_should_forward_hardware_io_errors(void) {
    enum CanCommunicationReturnCode process_rc;
    struct CanCommunicationNetworkConfig config = { .send = mock_can_send, .on_receive = mock_application_on_receive };
    struct CanCommunicationFrame frame = { .id = 0x77U, .length = 4U, .data = { 0x1, 0x2, 0x3, 0x4 } };

    mock_can_send_fake.return_val = CAN_COMM_RC_ERROR;
    can_communication_api_init(CAN_COMM_NET_INVERTER, &config);
    can_communication_api_add_to_tx(CAN_COMM_NET_INVERTER, &frame);

    process_rc = can_communication_api_process_tx(CAN_COMM_NET_INVERTER);

    TEST_ASSERT_EQUAL_MESSAGE(CAN_COMM_RC_TRANSMISSION_ERROR, process_rc, "Transmission module processing failed to turn peripheral IO errors into explicit transmission failure codes");
}
/*! \} */

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_can_communication_api_init_should_fail_on_null_config);
    RUN_TEST(test_can_communication_api_init_should_fail_on_missing_mandatory_send_callback);
    RUN_TEST(test_can_communication_api_init_should_fail_on_missing_mandatory_receive_callback);
    RUN_TEST(test_can_communication_api_init_should_fail_on_invalid_network_bounds);
    RUN_TEST(test_can_communication_api_init_should_succeed_with_correct_parameters);

    RUN_TEST(test_can_communication_api_add_to_rx_should_fail_if_uninitialized);
    RUN_TEST(test_can_communication_api_add_to_rx_should_fail_on_null_frame);
    RUN_TEST(test_can_communication_api_add_to_rx_should_fail_on_invalid_length);
    RUN_TEST(test_can_communication_api_add_to_rx_should_fail_on_invalid_network_bounds);
    RUN_TEST(test_can_communication_api_add_to_rx_should_fail_with_buffer_full);

    RUN_TEST(test_can_communication_api_process_rx_should_fail_if_uninitialized);
    RUN_TEST(test_can_communication_api_process_rx_should_fail_on_invalid_network_bounds);
    RUN_TEST(test_can_communication_api_process_rx_should_drain_entire_queue_to_dispatcher);
    RUN_TEST(test_can_communication_api_process_rx_should_forward_receive_handler_errors);

    RUN_TEST(test_can_communication_api_add_to_tx_should_fail_if_uninitialized);
    RUN_TEST(test_can_communication_api_add_to_tx_should_fail_on_null_frame);
    RUN_TEST(test_can_communication_api_add_to_tx_should_fail_on_invalid_length);
    RUN_TEST(test_can_communication_api_add_to_tx_should_fail_on_invalid_network_bounds);
    RUN_TEST(test_can_communication_api_add_to_tx_should_succeed);
    RUN_TEST(test_can_communication_api_add_to_tx_should_fail_with_buffer_full);

    RUN_TEST(test_can_communication_api_process_tx_should_fail_if_uninitialized);
    RUN_TEST(test_can_communication_api_process_tx_should_fail_on_invalid_network_bounds);
    RUN_TEST(test_can_communication_api_process_tx_should_flush_to_hardware);
    RUN_TEST(test_can_communication_api_process_tx_should_forward_hardware_io_errors);

    return UNITY_END();
}