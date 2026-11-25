#ifndef FLASHING_API_H
#define FLASHING_API_H

#include "flashing.h"

/*!
 * \brief Initialize the flashing module
 *
 * \param[in] watchdog_handler The global watchdog pool
 * \param[in] system_reset_callback Callback to the function that performs the system reset
 *
 * \retval FLASHING_RC_NULL_POINTER if either the pool or the reset callback are null
 * \retval FLASHING_RC_OK on a successful initialization
 */
enum FlashingReturnCode flashing_api_init(struct WatchdogHandler *watchdog_handler, flashing_system_reset_callback system_reset_callback);

/*!
 * \brief Perform a system reset
 *
 * \retval FLASHING_RC_NULL_POINTER if the reset callback is null
 * \retval FLASHING_RC_OK on a successful reset
 */
enum FlashingReturnCode flashing_api_perform_reset(void);

/*!
 * \brief Restart the flashing watchdog
 *
 * \retval FLASHING_RC_ERROR if the watchdog could not be restarted
 * \retval FLASHING_RC_OK on a successful restart
 *
 * \todo Merge with the pet function, taking the frame ID and "dispatching" internally
 */
enum FlashingReturnCode flashing_api_restart_watchdog(void);

/*!
 * \brief Pet the flashing watchdog
 *
 * \retval FLASHING_RC_ERROR if the watchdog could not be petted
 * \retval FLASHING_RC_OK on success
 *
 * \todo Merge with the restart function, taking the frame ID and "dispatching" internally
 */
enum FlashingReturnCode flashing_api_pet_watchdog(void);

/*!
 * \brief Check whether the flashing watchdog has timed out
 *
 * \return true if the flashing watchdog has timed out since the last restart, false otherwise
 */
bool flashing_api_is_timed_out(void);

/*!
 * \brief Check whether a flashing operation is in progress
 *
 * \return true if the module is currently flashing, false otherwise
 */
bool flashing_api_is_flashing(void);

/*!
 * \brief Set the flashing state
 *
 * \param[in] flashing true to mark a flashing operation as in progress, false to mark it as finished
 *
 * \retval FLASHING_RC_OK always
 */
enum FlashingReturnCode flashing_api_set_flashing(bool flashing);

#endif // FLASHING_API_H
