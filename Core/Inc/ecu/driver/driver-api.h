/*!
 * \file driver-api.h
 * \author Dorijan Di Zepp
 * \date 2026-05-14
 * \brief API for driver selection, readiness verification before R2D transition
 * and continuous safety monitoring during operation.
 */

#ifndef DRIVER_API_H
#define DRIVER_API_H

#include "driver.h"

/*!
 * \brief Initializes the driver handler by storing the driver type and necessary callbacks.
 * \note If the initialization is not successful, the previous configuration is left unchanged.
 * \note It is recommended to initialize the module with \ref DRIVER_TYPE_MANUAL by default. 
 * This establishes a **fail-safe default configuration**, ensuring that human control 
 * is the baseline state upon system boot or reset. This prevents the vehicle from 
 * defaulting to autonomous control (AS) without explicit, intentional selection.
 * \warning Initialization and driver switching are only permitted in the "Init" or "Idle" states. 
 * This ensures that the vehicle operates under a deterministic driver assignment in all 
 * subsequent operational states.
 * \param[in] driver_type The driver selected to operate the vehicle.
 * \param[in] wait_for_driver Callback to determine if the driver is ready to drive.
 * \param[in] continuous_check Callback to verify basic safety conditions during driving.
 * \retval DRIVER_RC_OK If the initialization was successful.
 * \retval DRIVER_RC_ERROR If a callback is \c NULL, the state is invalid or the driver type is not valid.
 */
enum DriverReturnCode driver_api_init(enum DriverType driver_type, driver_wait_for_driver_callback wait_for_driver, driver_continuous_check_callback continuous_check);

/*!
 * \brief Verifies if the driver is ready to drive.
 * \note The outcome is determined solely by the logic defined within the 
 * \ref driver_wait_for_driver_callback.
 * \note The result is typically used by the FSM to trigger a transition to the R2D state.
 * \retval DRIVER_RC_OK Driver meets all readiness conditions.
 * \retval DRIVER_RC_ERROR Driver is NOT ready or the internal callback is \c NULL.
 */
enum DriverReturnCode driver_api_is_driver_ready(void);

/*!
 * \brief Performs continuous safety monitoring during the driving phase.
 * \details This function verifies that critical operating conditions are met. 
 * Failure of these checks usually mandates immediate inverter deactivation for safety.
 * \note The result is intended for use by the FSM (specifically in the R2D state) 
 * to decide if the inverters or Autonomous System must be disabled without explicit user request.
 * \retval DRIVER_RC_OK All safety conditions are met; car is safe to drive.
 * \retval DRIVER_RC_ERROR A safety check failed or the callback is \c NULL; stop the vehicle.
 */
enum DriverReturnCode driver_api_continuous_check(void);

/*!
 * \brief Returns the currently selected driver type.
 * \return The active \ref DriverType (e.g. MANUAL or AS).
 */
enum DriverType driver_api_get_driver_type(void);

#endif