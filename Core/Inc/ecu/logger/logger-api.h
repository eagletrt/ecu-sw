/*!
 * \file logger-api.h
 * \author Dorijan Di Zepp
 * \date 2026-06-01
 * \brief API interface for initializing and writing to the system logger.
 */

#ifndef LOGGER_API_H
#define LOGGER_API_H

#include "logger.h"

/*!
 * \brief Initializes the logger storage and binds it to an active PAL context instance.
 * \note The struct PalHandler passed to this module should be dedicated solely to 
 * the physical interface used for logging (e.g. UART). 
 * If your system interfaces with separate physical protocols (such as a CAN bus), 
 * a distinct, separate `PalHandler` instance must be initialized for that peripheral 
 * to prevent transmission queue collisions and driver conflicts.
 * \param[in] pal_h Pointer to the initialized PAL configuration tracking block.
 * \retval LOGGER_RC_OK if setup succeeds.
 * \retval LOGGER_RC_ERROR if the input pointer is NULL.
 */
enum LoggerReturnCode logger_api_init(struct PalHandler *pal_h);

/*!
 * \brief Formally dispatches a formatted log message over the registered PAL UART interface.
 * \details Leverages C variable arguments to provide printf-style formatting.
 * \param[in] level Severity level of the outgoing record.
 * \param[in] format Standard printf-style format configuration string.
 * \param[in] ... Variable argument listing.
 * \retval LOGGER_RC_OK if the message was successfully queued and processed by PAL.
 * \retval LOGGER_RC_ERROR if formatting failed, or PAL encountered an IO/size constraint.
 */
enum LoggerReturnCode logger_api_log(enum LoggerLevel level, const char *format, ...);

#endif