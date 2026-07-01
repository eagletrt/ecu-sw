/*!
 * \file can-communication-router-api.h
 * \author Dorijan Di Zepp
 * \date 2026-07-01
 * \brief Routing and deserialization hooks for incoming ECU network traffic.
 */

#ifndef CAN_COMMUNICATION_ROUTER_API_H
#define CAN_COMMUNICATION_ROUTER_API_H

#include "can-communication.h"

/*!
 * \brief Function for incoming CAN frames on primary network.
 * \param[in] frame The frame just removed from the RX queue.
 * \retval CAN_COMMUNICATION_RC_OK on success.
 * \retval CAN_COMMUNICATION_RC_RECEIVE_HANDLER_ERROR if dispatch fails.
 */
enum CanCommunicationReturnCode can_communication_router_api_receive_primary(struct CanCommunicationFrame *frame);

/*!
 * \brief Function for incoming CAN frames on secondary network.
 * \param[in] frame The frame just removed from the RX queue.
 * \retval CAN_COMMUNICATION_RC_OK on success.
 * \retval CAN_COMMUNICATION_RC_RECEIVE_HANDLER_ERROR if dispatch fails.
 */
enum CanCommunicationReturnCode can_communication_router_api_receive_secondary(struct CanCommunicationFrame *frame);

/*!
 * \brief Function for incoming CAN frames on inverter network.
 * \param[in] frame The frame just removed from the RX queue.
 * \retval CAN_COMMUNICATION_RC_OK on success.
 * \retval CAN_COMMUNICATION_RC_RECEIVE_HANDLER_ERROR if dispatch fails.
 */
enum CanCommunicationReturnCode can_communication_router_api_receive_inverter(struct CanCommunicationFrame *frame);

#endif