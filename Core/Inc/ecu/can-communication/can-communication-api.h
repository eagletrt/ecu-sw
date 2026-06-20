/*!
 * \file can-communication-api.h
 * \author Dorijan Di Zepp
 * \date 2026-06-20
 * \brief Generic API interface for routing abstracted CAN frames via PAL.
 */

#ifndef CAN_COMMUNICATION_API_H
#define CAN_COMMUNICATION_API_H

#include "can-communication.h"

/*!
 * \brief Initializes a single specific CAN network channel within the module.
 * \details This configures the network's local PAL handler, allocates memory via the 
 * internal arena and binds user-supplied hardware transmission and message routing hooks.
 *
 * \param[in] network The target physical CAN network to initialize.
 * \param[in] config Configuration structure containing required callback hooks and optional critical sections.
 *
 * \retval CAN_COMM_RC_OK If the network channel is successfully setup and activated.
 * \retval CAN_COMM_RC_NULL_POINTER If \p config is NULL or any of its mandatory function hooks are NULL.
 * \retval CAN_COMM_RC_INVALID_ARGUMENT If \p network matches or exceeds CAN_COMM_NET_COUNT or is already initialized.
 * \retval CAN_COMM_RC_ERROR If internal allocation or PAL ring buffer creation fails.
 */
enum CanCommunicationReturnCode can_communication_api_init(enum CanCommunicationNetwork network,
                                                           const struct CanCommunicationNetworkConfig *config);

/*!
 * \brief Pushes an abstracted data frame into a specific network's local TX buffer queue.
 *
 * \details The upper application layer runs serialization protocols first to assemble 
 * a complete `CanCommunicationFrame` structure before invoking this API to stage it for transmission.
 *
 * \param[in] network The targeted physical CAN network the frame is destined for.
 * \param[in] frame Pointer to the completely formed frame data structure to stage.
 *
 * \retval CAN_COMM_RC_OK If the frame was successfully appended to the queue.
 * \retval CAN_COMM_RC_NULL_POINTER If the provided \p frame pointer evaluates to NULL.
 * \retval CAN_COMM_RC_INVALID_ARGUMENT If \p network matches or exceeds CAN_COMM_NET_COUNT.
 * \retval CAN_COMM_RC_NOT_INITIALIZED If called on a network channel whose initialization was never executed.
 * \retval CAN_COMM_RC_INVALID_LENGTH If \p frame->length exceeds CAN_COMM_COMMUNICATION_FRAME_DATA_SIZE.
 * \retval CAN_COMM_RC_BUFFER_FULL If the underlying transmission queue is completely saturated.
 * \retval CAN_COMM_RC_ERROR If a low-level structural exception occurs inside the PAL ring buffer.
 */
enum CanCommunicationReturnCode can_communication_api_add_to_tx(enum CanCommunicationNetwork network,
                                                                const struct CanCommunicationFrame *frame);

/*!
 * \brief Pushes a raw frame received from a peripheral bus into a specific network's local RX buffer queue.
 * \note Call this function directly inside low-level hardware ISR handler routines.
 *
 * \param[in] network The physical CAN network where the incoming packet arrived.
 * \param[in] frame Pointer to the data structure holding the raw ID, length and payload metrics.
 *
 * \retval CAN_COMM_RC_OK If the frame was safely staged for future state processing.
 * \retval CAN_COMM_RC_NULL_POINTER If the provided \p frame pointer evaluates to NULL.
 * \retval CAN_COMM_RC_INVALID_ARGUMENT If \p network matches or exceeds CAN_COMM_NET_COUNT.
 * \retval CAN_COMM_RC_NOT_INITIALIZED If called on a network channel whose initialization was never executed.
 * \retval CAN_COMM_RC_INVALID_LENGTH If \p frame->length exceeds CAN_COMM_COMMUNICATION_FRAME_DATA_SIZE.
 * \retval CAN_COMM_RC_BUFFER_FULL If the reception queue is saturated.
 * \retval CAN_COMM_RC_ERROR If an internal verification error occurs within the PAL layer.
 */
enum CanCommunicationReturnCode can_communication_api_add_to_rx(enum CanCommunicationNetwork network,
                                                                const struct CanCommunicationFrame *frame);

/*!
 * \brief Flushes and drains the entire TX queue of a specific network out to physical hardware.
 *
 * \details This flushes staged frames sequentially, passing them to the user-supplied 
 * hardware transmitter callback. It loops continuously until the queue reports empty.
 *
 * \param[in] network The physical CAN network whose transmission queue should be flushed.
 *
 * \retval CAN_COMM_RC_OK If all queued elements were completely flushed out.
 * \retval CAN_COMM_RC_INVALID_ARGUMENT If \p network matches or exceeds CAN_COMM_NET_COUNT.
 * \retval CAN_COMM_RC_NOT_INITIALIZED If the targeted network was never setup.
 * \retval CAN_COMM_RC_TRANSMISSION_ERROR If the underlying hardware driver callback reports an operation failure.
 * \retval CAN_COMM_RC_ERROR If a low-level internal driver crash occurs.
 */
enum CanCommunicationReturnCode can_communication_api_process_tx(enum CanCommunicationNetwork network);

/*!
 * \brief Drains the entire RX queue of a specific network, dispatching frames to the application layer.
 *
 * \details This pops frames out of the network's reception buffer and routes them through 
 * the registered `on_receive` deserialization engine callback. It processes elements until empty.
 *
 * \param[in] network The physical CAN network whose reception queue should be processed.
 *
 * \retval CAN_COMM_RC_OK If all received messages were popped and processed cleanly.
 * \retval CAN_COMM_RC_INVALID_ARGUMENT If \p network matches or exceeds CAN_COMM_NET_COUNT.
 * \retval CAN_COMM_RC_NOT_INITIALIZED If the targeted network was never setup.
 * \retval CAN_COMM_RC_RECEIVE_HANDLER_ERROR If the user routing callback fails to process an extracted frame.
 * \retval CAN_COMM_RC_ERROR If a PAL internal pipeline fault is encountered.
 */
enum CanCommunicationReturnCode can_communication_api_process_rx(enum CanCommunicationNetwork network);

#endif