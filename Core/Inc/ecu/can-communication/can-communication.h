/*!
 * \file can-communication.h
 * \author Dorijan Di Zepp
 * \date 2026-06-13
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
 * \brief Identifiers for the physical CAN networks used by the ECU.
 */
enum CanCommunicationNode {
    CAN_COMM_NODE_1,    /*!< Target the CAN 1 bus. */
    CAN_COMM_NODE_2,    /*!< Target the CAN 2 bus. */
    CAN_COMM_NODE_3,    /*!< Target the CAN 3 bus. */
    CAN_COMM_NODE_COUNT /*!< Sentinel value used exclusively to verify enum parameter validity. */
};

/*!
 * \brief Internal context state handler container.
 * \note This struct maintains 3 distinct PAL pointers to ensure entirely 
 * separate TX/RX buffering queues for the 3 distinct CAN peripherals.
 */
struct CanCommunicationHandler {
    struct PalHandler *pal_can1; /*!< Referenced PAL tracking instance for CAN 1 routing. */
    struct PalHandler *pal_can2; /*!< Referenced PAL tracking instance for CAN 2 routing. */
    struct PalHandler *pal_can3; /*!< Referenced PAL tracking instance for CAN 3 routing. */
};

#endif