/*!
 * \file logger-api.c
 * \author Dorijan Di Zepp
 * \date 2026-06-01
 * \brief API execution handling for logging records.
 */

#include "logger-api.h"
#include "eagletrt-api.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define LOGGER_MAX_LINE_SIZE (64U)

/*!
 * \brief Internal module handler.
 * \details Hidden from external linkage to enforce API-only access.
 */
EAGLETRT_STATIC struct LoggerHandler logger_handler;

enum LoggerReturnCode logger_api_init(struct PalHandler *pal_h) {
    if (pal_h == NULL) {
        return LOGGER_RC_NULL_POINTER;
    }

    logger_handler.pal_handler = pal_h;
    return LOGGER_RC_OK;
}

enum LoggerReturnCode logger_api_log(enum LoggerLevel level, const char *format, ...) {
    if (logger_handler.pal_handler == NULL || format == NULL) {
        return LOGGER_RC_NULL_POINTER;
    }

    char final_buffer[LOGGER_MAX_LINE_SIZE];
    int offset = 0;

    // Prepend metadata severity tags
    switch (level) {
        case LOGGER_LEVEL_DEBUG:
            offset = snprintf(final_buffer, sizeof(final_buffer), "[DEBUG] ");
            break;
        case LOGGER_LEVEL_INFO:
            offset = snprintf(final_buffer, sizeof(final_buffer), "[INFO] ");
            break;
        case LOGGER_LEVEL_WARN:
            offset = snprintf(final_buffer, sizeof(final_buffer), "[WARN] ");
            break;
        case LOGGER_LEVEL_ERROR:
            offset = snprintf(final_buffer, sizeof(final_buffer), "[ERROR] ");
            break;
        default:
            offset = snprintf(final_buffer, sizeof(final_buffer), "[LOG] ");
            break;
    }

    // Verify no anomalies or truncations occurred during tag placement
    if (offset < 0 || offset >= (int)sizeof(final_buffer)) {
        return LOGGER_RC_TRANSMISSION_ERROR;
    }

    // Process variable args into the remaining space of the local buffer
    va_list args;
    va_start(args, format);
    int body_len = vsnprintf(final_buffer + offset, sizeof(final_buffer) - (size_t)offset, format, args);
    va_end(args);

    if (body_len < 0) {
        return LOGGER_RC_TRANSMISSION_ERROR; // Format parsing exception
    }

    int total_len = offset + body_len;

    // Determine transmission size including the string null terminator to match PAL style
    uint32_t tx_bytes = (total_len >= (int)LOGGER_MAX_LINE_SIZE) ? (uint32_t)LOGGER_MAX_LINE_SIZE : (uint32_t)(total_len + 1);

    // Queue the formatted record into PAL and collapse lower-level parameters into abstract behaviors
    enum PalReturnCode pal_rc = pal_api_add_to_tx_queue(logger_handler.pal_handler, final_buffer, tx_bytes);
    if (pal_rc != PAL_RC_OK) {
        if (pal_rc == PAL_RC_QUEUE_FULL) {
            return LOGGER_RC_BUFFER_FULL;
        }
        return LOGGER_RC_TRANSMISSION_ERROR;
    }

    // Process/Flush the queue immediately so diagnostics output synchronously
    pal_rc = pal_api_process_tx(logger_handler.pal_handler);
    if (pal_rc != PAL_RC_OK) {
        if (pal_rc == PAL_RC_QUEUE_FULL) {
            return LOGGER_RC_BUFFER_FULL;
        }
        return LOGGER_RC_TRANSMISSION_ERROR;
    }

    return LOGGER_RC_OK;
}