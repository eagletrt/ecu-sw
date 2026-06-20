/*!
 * \file can-communication-router.h
 * \author Dorijan Di Zepp
 * \date 2026-06-20
 * \brief Routing and deserialization hooks for incoming ECU network traffic.
 */

#ifndef CAN_COMMUNICATION_ROUTER_H
#define CAN_COMMUNICATION_ROUTER_H

#include "can-communication.h"

/*!
 * \brief Router function for incoming CAN frames on the Primary network.
 * \details Registered as the `on_receive` hook during primary network init.
 */
enum CanCommunicationReturnCode can_communication_router_receive_primary(const struct CanCommunicationFrame *frame);

/*!
 * \brief Router function for incoming CAN frames on the Secondary network.
 * \details Registered as the `on_receive` hook during secondary network init.
 */
enum CanCommunicationReturnCode can_communication_router_receive_secondary(const struct CanCommunicationFrame *frame);

/*!
 * \brief Router function for incoming CAN frames on the Inverter powertrain network.
 * \details Registered as the `on_receive` hook during inverter network init.
 */
enum CanCommunicationReturnCode can_communication_router_receive_inverter(const struct CanCommunicationFrame *frame);

#endif