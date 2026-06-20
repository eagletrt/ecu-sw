/*!
 * \file can-communication.h
 * \author Dorijan Di Zepp
 * \date 2026-06-20
 * \brief Data types, enums and structures for the CAN communication module.
 */

#ifndef CAN_COMMUNICATION_H
#define CAN_COMMUNICATION_H

#include "pal.h"
#include "arena-allocator.h"
#include <stdint.h>
#include <stdbool.h>

/*!
 * \brief Maximum payload size of a classic-CAN frame, in bytes.
 */
#define CAN_COMMUNICATION_FRAME_DATA_SIZE (8U)

/*!
 * \brief Size of the per-network reception queue.
 */
#define CAN_COMMUNICATION_RX_QUEUE_CAPACITY (128U)

/*!
 * \brief Size of the per-network transmission queue.
 */
#define CAN_COMMUNICATION_TX_QUEUE_CAPACITY (128U)

/*!
 * \brief System return and status codes for CAN communication operations.
 */
enum CanCommunicationReturnCode {
    CAN_COMM_RC_OK,                    /*!< Operation completed successfully. */
    CAN_COMM_RC_ERROR,                 /*!< Operation not completed due to an internal system error. */
    CAN_COMM_RC_NULL_POINTER,          /*!< An invalid NULL pointer was passed to the module. */
    CAN_COMM_RC_INVALID_ARGUMENT,      /*!< An out-of-bounds enum value or invalid CAN node was targeted. */
    CAN_COMM_RC_INVALID_LENGTH,        /*!< Frame payload length exceeds CAN_COMMUNICATION_FRAME_DATA_SIZE. */
    CAN_COMM_RC_NOT_INITIALIZED,       /*!< API invoked on a network channel that has not been initialized. */
    CAN_COMM_RC_TRANSMISSION_ERROR,    /*!< Downstream PAL physical hardware transmission failed. */
    CAN_COMM_RC_RECEIVE_HANDLER_ERROR, /*!< The user-supplied receive dispatch callback reported a failure. */
    CAN_COMM_RC_BUFFER_FULL,           /*!< The targeted buffer queue is completely full. */
    CAN_COMM_RC_BUFFER_EMPTY           /*!< No elements available to pull from the buffer queue. */
};

/*!
 * \brief Identifiers for the physical CAN networks managed by the ECU.
 */
enum CanCommunicationNetwork {
    CAN_COMM_NET_PRIMARY,   /*!< Target the Primary CAN bus. */
    CAN_COMM_NET_SECONDARY, /*!< Target the Secondary CAN bus. */
    CAN_COMM_NET_INVERTER,  /*!< Target the Inverters CAN bus. */
    CAN_COMM_NET_COUNT      /*!< Sentinel value used exclusively to verify network parameter validity. */
};

/*!
 * \brief Hardware-agnostic container representing a single raw CAN frame payload.
 */
struct CanCommunicationFrame {
    uint32_t id;                                     /*!< CAN identifier (Standard 11-bit or Extended 29-bit) */
    uint8_t length;                                  /*!< Count of valid data bytes within the array (0..8) */
    uint8_t data[CAN_COMMUNICATION_FRAME_DATA_SIZE]; /*!< Raw payload byte buffer */
};

/*!
 * \brief Callback signatures used to cleanly pass function hooks across code boundaries.
 */
typedef enum CanCommunicationReturnCode (*CanCommunicationSendCallback)(const struct CanCommunicationFrame *frame);
typedef enum CanCommunicationReturnCode (*CanCommunicationReceiveCallback)(const struct CanCommunicationFrame *frame);
typedef void (*CanCommunicationCriticalSectionCallback)(void);

/*!
 * \brief Configuration parameters required to open a single bus channel.
 */
struct CanCommunicationNetworkConfig {
    CanCommunicationSendCallback send;                /*!< Peripheral driver frame transmitter function hook */
    CanCommunicationReceiveCallback on_receive;       /*!< Deserialization routing engine function hook */
    CanCommunicationCriticalSectionCallback cs_enter; /*!< Optional: Thread-safety block lock initializer (May be \c NULL) */
    CanCommunicationCriticalSectionCallback cs_exit;  /*!< Optional: Thread-safety block lock release initializer (May be \c NULL) */
};

/*!
 * \brief Explicit tracking layout block dedicated to a singular bus network slot.
 */
struct CanCommunicationNetworkState {
    struct PalHandler pal;                      /*!< Local PAL tracking instance allocated for this network's queues */
    CanCommunicationSendCallback send;          /*!< Buffered reference pointing to driver transmitter */
    CanCommunicationReceiveCallback on_receive; /*!< Buffered reference pointing to deserializer */
};

/*!
 * \brief Internal context state handler container.
 */
struct CanCommunicationHandler {
    struct ArenaAllocatorHandler arena;                               /*!< Internal memory arena shared across network handlers */
    struct CanCommunicationNetworkState networks[CAN_COMM_NET_COUNT]; /*!< State structures managing individual physical networks */
};

#endif