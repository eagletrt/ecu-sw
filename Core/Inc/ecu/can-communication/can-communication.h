/*!
 * \file can-communication.h
 * \author Dorijan Di Zepp
 * \date 2026-06-14
 * \brief Data types, enums and structures for the CAN communication module.
 */

#ifndef CAN_COMMUNICATION_H
#define CAN_COMMUNICATION_H

#include "pal-api.h"

/*!
 * \brief System return and status codes for CAN communication operations.
 */
enum CanCommunicationReturnCode {
    CAN_COMM_RC_OK,                 /*!< Operation completed successfully. */
    CAN_COMM_RC_ERROR,              /*!< Operation not completed due to an internal system error. */
    CAN_COMM_RC_NULL_POINTER,       /*!< An invalid NULL pointer was passed to the module. */
    CAN_COMM_RC_INVALID_ARGUMENT,   /*!< An out-of-bounds enum value or invalid CAN node was targeted. */
    CAN_COMM_RC_TRANSMISSION_ERROR, /*!< Downstream PAL physical hardware transmission failed or generic error occurred. */
    CAN_COMM_RC_BUFFER_FULL         /*!< The targeted PAL transmission queue for a CAN node is completely full. */
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
 * \brief Internal context state handler container.
 * \note This struct maintains distinct PAL pointers to ensure entirely 
 * separate TX/RX buffering queues for the individual CAN networks.
 */
struct CanCommunicationHandler {
    struct PalHandler *primary_pal;   /*!< Referenced PAL tracking instance for the Primary network routing. */
    struct PalHandler *secondary_pal; /*!< Referenced PAL tracking instance for the Secondary network routing. */
    struct PalHandler *inverter_pal;  /*!< Referenced PAL tracking instance for the Inverter network routing. */
};

#endif