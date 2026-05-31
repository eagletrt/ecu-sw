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

/*!
 * \brief System logger return/status codes.
 */
enum LoggerReturnCode {
    LOGGER_RC_OK = 0,
    LOGGER_RC_ERROR = 1
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