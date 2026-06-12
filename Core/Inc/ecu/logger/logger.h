/*!
 * \file logger.h
 * \author Dorijan Di Zepp
 * \date 2026-06-12
 * \brief Data types, enums and structures for the system logger module.
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include "pal-api.h"

/*!
 * \brief System logger return/status codes.
 */
enum LoggerReturnCode {
    LOGGER_RC_OK,                 /*!< Operation completed successfully. */
    LOGGER_RC_ERROR,              /*!< Operation not completed due to an internal error. */
    LOGGER_RC_NULL_POINTER,       /*!< An invalid NULL pointer was passed directly to the logger. */
    LOGGER_RC_TRANSMISSION_ERROR, /*!< Downstream PAL physical hardware transmission failed or generic error occurred. */
    LOGGER_RC_BUFFER_FULL         /*!< The logging medium or underlying PAL transmission queue is completely full. */
};

/*!
 * \brief Operational states for the logger.
 */
enum LoggerState {
    LOGGER_STATE_DISABLED, /*!< The logger will ignore any logging request. */
    LOGGER_STATE_ENABLED,  /*!< The logger will try to log any information passed. */
    LOGGER_STATE_COUNT,    /*!< Sentinel value used exclusively to verify enum parameter validity. */
};

/*!
 * \brief Severity level for logging records.
 */
enum LoggerLevel {
    LOGGER_LEVEL_DEBUG, /*!< Verbose diagnostics for developer use. */
    LOGGER_LEVEL_INFO,  /*!< Routine operational updates. */
    LOGGER_LEVEL_WARN,  /*!< Non-fatal anomalies or timing retries. */
    LOGGER_LEVEL_ERROR  /*!< Fatal system faults. */
};

/*!
 * \brief Internal context state handler container.
 * \note The \ref pal_handler member points to a unique PAL interface dedicated 
 * to logging/console output (typically mapped to a specific UART channel).
 */
struct LoggerHandler {
    struct PalHandler *pal_handler; /*!< Referenced PAL tracking instance for UART routing. */
    enum LoggerState state;         /*!< Current runtime filtering state of the logger. */
};

#endif