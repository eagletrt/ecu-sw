/*!
 * \file as_driver_api.h
 * \author Dorijan Di Zepp
 * \date 2026-05-31
 * \brief API interface for manipulating the global AS Driver context.
 */

#ifndef AS_DRIVER_API_H
#define AS_DRIVER_API_H

#include "as-driver.h"

/*!
 * \brief Initializes the private AS Driver state to default values.
 * \details Clears all internal metrics, flags and binds the mandatory
 * low-level pneumatic relief callback to the internal instance.
 * \param[in] air_callback  Function pointer pointing to the PAL-level actuator.
 * \retval AS_DRIVER_RC_OK on successful initialization.
 * \retval AS_DRIVER_RC_ERROR if the callback pointer is \c NULL.
 */
enum ASDriverReturnCode as_driver_api_init(air_release_from_line_callback air_callback);

/*!
 * \brief           Triggers the release of air from a specified pneumatic line.
 * \details         Invokes the underlying functional safety callback registered 
 * during module initialization to change pneumatic state.
 * \param[in]       line  The target pneumatic line to vent.
 * \retval          AS_DRIVER_RC_OK if the callback was successfully executed.
 * \retval          AS_DRIVER_RC_ERROR if the module is uninitialized or the callback is missing.
 */
enum ASDriverReturnCode as_driver_api_release_air(enum ASDriverAirLine line);

/*!
 * \brief Sets the currently selected autonomous mission discipline.
 * \param[in] mission The target \ref ASDriverMission discipline enum value.
 */
void as_driver_api_set_mission(enum ASDriverMission mission);

/*!
 * \brief Gets the currently selected autonomous mission discipline.
 * \return The active \ref ASDriverMission enum value.
 */
enum ASDriverMission as_driver_api_get_mission(void);

/*!
 * \brief Updates the private system pressure profiles.
 * \param[in] new_press  Pointer to the latest pressure data block from PAL.
 * \retval AS_DRIVER_RC_OK if the data was successfully copied.
 * \retval AS_DRIVER_RC_ERROR if the input pointer is \c NULL.
 */
enum ASDriverReturnCode as_driver_api_set_pressures(const struct ASDriverPressures *new_press);

/*!
 * \brief           Retrieves the current system pressure profiles.
 * \param[out]      out_press  Pointer to a pressure structure to populate with current data.
 * \retval          AS_DRIVER_RC_OK if the data was successfully retrieved.
 * \retval          AS_DRIVER_RC_ERROR if the output pointer is \c NULL.
 */
enum ASDriverReturnCode as_driver_api_get_pressures(struct ASDriverPressures *out_press);

/*!
 * \brief           Updates the private mechanical sensor displacement metrics.
 * \param[in]       new_mech   Pointer to the latest mechanical metrics block from PAL.
 * \retval          AS_DRIVER_RC_OK if the data was successfully copied.
 * \retval          AS_DRIVER_RC_ERROR if the input pointer is \c NULL.
 */
enum ASDriverReturnCode as_driver_api_set_mechanical_sensors(const struct ASDriverMechanicalSensors *new_mech);

/*!
 * \brief           Retrieves the current mechanical sensor displacement metrics.
 * \param[out]      out_mech   Pointer to a mechanical structure to populate with current data.
 * \retval          AS_DRIVER_RC_OK if the data was successfully retrieved.
 * \retval          AS_DRIVER_RC_ERROR if the output pointer is \c NULL.
 */
enum ASDriverReturnCode as_driver_api_get_mechanical_sensors(struct ASDriverMechanicalSensors *out_mech);

/*!
 * \brief           Sets the physical Autonomous System Master Switch (ASMS) status flag.
 * \param[in]       status  True if the ASMS is switched on.
 */
void as_driver_api_set_asms_on(bool status);

/*!
 * \brief           Gets the physical Autonomous System Master Switch (ASMS) status flag.
 * \return          True if the ASMS is switched on.
 */
bool as_driver_api_get_asms_on(void);

/*!
 * \brief           Sets the mission started status flag.
 * \param[in]       status  True if the autonomous mission run has begun.
 */
void as_driver_api_set_mission_started(bool status);

/*!
 * \brief           Gets the mission started status flag.
 * \return          True if the autonomous mission run has begun.
 */
bool as_driver_api_get_mission_started(void);

/*!
 * \brief           Sets the hardware pipeline watchdog operational handshake flag.
 * \param[in]       status  True if the computing core handshake succeeded.
 */
void as_driver_api_set_watchdog_worked(bool status);

/*!
 * \brief           Gets the hardware pipeline watchdog operational handshake flag.
 * \return          True if the computing core handshake succeeded.
 */
bool as_driver_api_get_watchdog_worked(void);

/*!
 * \brief           Sets the cyclic timing execution watchdog verification flag.
 * \param[in]       status  True to trigger a watchdog evaluation check.
 */
void as_driver_api_set_watchdog_check(bool status);

/*!
 * \brief           Gets the cyclic timing execution watchdog verification flag.
 * \return          True if a watchdog evaluation check is requested.
 */
bool as_driver_api_get_watchdog_check(void);

/*!
 * \brief           Sets the physical Tractive System Master Switch (TSMS) status flag.
 * \param[in]       status  True if the TSMS is switched on.
 */
void as_driver_api_set_tsms_on(bool status);

/*!
 * \brief           Gets the physical Tractive System Master Switch (TSMS) status flag.
 * \return          True if the TSMS is switched on.
 */
bool as_driver_api_get_tsms_on(void);

/*!
 * \brief           Sets the physical Shutdown Circuit (SDC) closed safety status flag.
 * \param[in]       status  True if the SDC logic loop is closed/complete.
 */
void as_driver_api_set_sdc_closed(bool status);

/*!
 * \brief           Gets the physical Shutdown Circuit (SDC) closed safety status flag.
 * \return          True if the SDC logic loop is closed/complete.
 */
bool as_driver_api_get_sdc_closed(void);

/*!
 * \brief           Sets the Remote Emergency System (RES) execution 'GO' command flag.
 * \param[in]       status  True if the wireless base station transmits a go signal.
 */
void as_driver_api_set_res_go(bool status);

/*!
 * \brief           Gets the Remote Emergency System (RES) execution 'GO' command flag.
 * \return          True if the wireless base station transmits a go signal.
 */
bool as_driver_api_get_res_go(void);

/*!
 * \brief           Sets the Remote Emergency System (RES) emergency cutoff flag.
 * \param[in]       status  True if the operator triggers a remote wireless abort.
 */
void as_driver_api_set_res_emergency(bool status);

/*!
 * \brief           Gets the Remote Emergency System (RES) emergency cutoff flag.
 * \return          True if the operator triggers a remote wireless abort.
 */
bool as_driver_api_get_res_emergency(void);

/*!
 * \brief           Sets the Emergency Brake System (EBS) active/tripped flag.
 * \param[in]       status  Indicate wether the EBS has been activated or not.
 */
void as_driver_api_set_ebs_active(bool status);

/*!
 * \brief           Gets the Emergency Brake System (EBS) active/tripped flag.
 * \return          True if EBS has been activated.
 */
bool as_driver_api_get_ebs_active(void);

/*!
 * \brief           Sets the vehicle standstill status flag.
 * \param[in]       is_standstill  True if the car is standstill. False otherwise.
 */
void as_driver_api_set_standstill(bool is_standstill);

/*!
 * \brief           Gets the vehicle standstill status flag.
 * \return          True if zero velocity is confirmed. False is the car is not standstill.
 */
bool as_driver_api_get_standstill(void);

#endif