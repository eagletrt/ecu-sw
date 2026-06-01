/*!
 * \file logger.h
 * \author Dorijan Di Zepp
 * \date 2026-06-01
 * \brief Data types, enums and structures for the system logger module.
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include "pal-api.h"

/**
 * \brief System logger return/status codes.
 */
enum LoggerReturnCode {
    LOGGER_RC_OK,                   /*!< Operation completed successfully. */
    LOGGER_RC_NULL_POINTER,         /*!< An invalid NULL pointer was passed directly to the logger. */
    LOGGER_RC_NOT_INITIALIZED,      /*!< Logging was attempted before initializing the module. */
    LOGGER_RC_FORMAT_ERROR,         /*!< String parsing or vsnprintf formatting failed. */
    LOGGER_RC_PAL_INVALID_ARGUMENT, /*!< PAL rejected parameter data provided by the logger. */
    LOGGER_RC_PAL_NULL_POINTER,     /*!< Downstream PAL encountered an unexpected NULL pointer. */
    LOGGER_RC_PAL_IO_ERROR,         /*!< Downstream PAL physical hardware transmission failed. */
    LOGGER_RC_PAL_QUEUE_FULL,       /*!< The PAL transmission queue is completely full. */
    LOGGER_RC_PAL_MESSAGE_TOO_BIG,  /*!< Formatted string exceeded internal PAL message limits. */
    LOGGER_RC_PAL_GENERIC_ERROR     /*!< Fallback for unhandled or unexpected PAL return codes. */
};

/*!
 * \brief Severity level for logging records.
 */
enum LoggerLevel {
    LOGGER_LEVEL_DEBUG = 0, /*!< Verbose diagnostics for developer use. */
    LOGGER_LEVEL_INFO = 1,  /*!< Routine operational updates. */
    LOGGER_LEVEL_WARN = 2,  /*!< Non-fatal anomalies or timing retries. */
    LOGGER_LEVEL_ERROR = 3  /*!< Fatal system faults. */
};

/*!
 * \brief Internal context state handler container.
 * \note The \ref pal_handler member points to a unique PAL interface dedicated 
 * to logging/console output (typically mapped to a specific UART channel).
 */
struct LoggerHandler {
    struct PalHandler *pal_handler; /*!< Referenced PAL tracking instance for UART routing. */
};

#endif