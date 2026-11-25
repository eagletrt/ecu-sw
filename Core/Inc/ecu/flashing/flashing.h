#ifndef FLASHING_H
#define FLASHING_H

#include "watchdogs.h"
#include "can-primary.h"

/*!
 * \brief Lowest CAN frame ID belonging to the flashing protocol 
 */
constexpr uint32_t flashing_can_min_frame_id = CAN_PRIMARY_MESSAGE_FRAME_ID_ECUFLASHINGTX;

/*!
 * \brief Highest CAN frame ID belonging to the flashing protocol 
 */
constexpr uint32_t flashing_can_max_frame_id = CAN_PRIMARY_MESSAGE_FRAME_ID_STEERINGBOARDFLASHINGRX;

/*!
 * \brief Callback that performs the system reset
 *
 * Provided by the application at init time and invoked by flashing_api_perform_reset().
 */
typedef void (*flashing_system_reset_callback)(void);

/*!
 * \brief Return codes of the flashing module
 */
enum FlashingReturnCode : uint8_t {
    FLASHING_RC_OK,           /*!< Operation completed successfully */
    FLASHING_RC_NULL_POINTER, /*!< A required pointer or callback was NULL */
    FLASHING_RC_ERROR,        /*!< Generic error, e.g. a watchdog operation failed */
};

/*!
 * \brief Internal state of the flashing module
 */
struct FlashingHandler {
    struct WatchdogHandler *watchdog_handler;    /*!< Global watchdog pool the flashing watchdog belongs to */
    struct Watchdog watchdog;                    /*!< Watchdog supervising the flashing operation */
    flashing_system_reset_callback system_reset; /*!< Callback used to perform the system reset */
    bool is_flashing;                            /*!< True while a flashing operation is in progress */
    bool is_timed_out;                           /*!< True if the flashing watchdog has timed out since the last restart */
};

#endif // FLASHING_H
