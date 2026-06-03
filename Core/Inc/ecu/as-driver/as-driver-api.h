/*!
 * \file as-driver-api.h
 * \author Dorijan Di Zepp
 * \date 2026-06-04
 * \brief API interface for manipulating the global AS Driver handler.
 */

#ifndef AS_DRIVER_API_H
#define AS_DRIVER_API_H

#include "as-driver.h"

/*!
 * \brief Initializes the AS driver handler to default values.
 * \details Clears all internal metrics, flags and binds the mandatory
 * low-level pneumatic relief callback to the internal instance.
 * \param[in] air_callback Function pointer pointing to the hw-level actuator.
 * \retval AS_DRIVER_RC_OK on successful initialization.
 * \retval AS_DRIVER_RC_ERROR if the callback pointer is \c NULL.
 */
enum ASDriverReturnCode as_driver_api_init(air_release_from_line_callback air_callback);

/*!
 * \brief Triggers the release of air from a specified pneumatic line.
 * \details Invokes the underlying functional safety callback registered 
 * during module initialization to change pneumatic state.
 * \param[in] line The target pneumatic line to vent.
 * \retval AS_DRIVER_RC_OK if the callback was successfully executed.
 * \retval AS_DRIVER_RC_ERROR if the callback is missing or it was not possible to actuate the line specified.
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
 * \brief Updates a specific system pressure point.
 * \param[in] index The target \ref ASDriverBrakePressure sensor identifier.
 * \param[in] pressure The physical pressure value to assign (bar).
 */
void as_driver_api_set_brake_pressure(enum ASDriverBrakePressure index, float pressure);

/*!
 * \brief Updates the entire system pressure using an array reference.
 * \details Copies a flat buffer block into the internal module tracking structure.
 * \param[in] pressures An array of floats containing exactly \ref AS_DRIVER_BRAKE_PRESSURE_COUNT elements.
 * \retval AS_DRIVER_RC_OK if the data block was successfully updated.
 * \retval AS_DRIVER_RC_ERROR if the input pointer is \c NULL.
 */
enum ASDriverReturnCode as_driver_api_set_all_brake_pressures(const float pressures[AS_DRIVER_BRAKE_PRESSURE_COUNT]);

/*!
 * \brief Retrieves a specific system pressure point by value.
 * \param[in] index The target \ref ASDriverBrakePressure sensor identifier.
 * \return The physical pressure value stored (bar).
 */
float as_driver_api_get_brake_pressure(enum ASDriverBrakePressure index);

/*!
 * \brief Retrieves a read-only pointer to the entire brake pressure array.
 * \return A constant pointer to the first element of an array containing 
 */
const float *as_driver_api_get_all_brake_pressures(void);

/*!
 * \brief Updates a specific mechanical sensor displacement/force metric by value.
 * \param[in] index The target \ref ASDriverMechanicalSensors sensor identifier.
 * \param[in] value The raw metrics value to assign (mm / N).
 */
void as_driver_api_set_mechanical_sensor(enum ASDriverMechanicalSensor index, float value);

/*!
 * \brief Updates all mechanical sensor metrics using an array reference.
 * \details Copies a flat buffer block into the internal module tracking structure.
 * \param[in] sensors An array of floats containing exactly \ref AS_DRIVER_MECHANICAL_SENSORS_COUNT elements.
 * \retval AS_DRIVER_RC_OK if the data block was successfully updated.
 * \retval AS_DRIVER_RC_ERROR if the input pointer is \c NULL.
 */
enum ASDriverReturnCode as_driver_api_set_all_mechanical_sensors(const float sensors[AS_DRIVER_MECHANICAL_SENSOR_COUNT]);

/*!
 * \brief Retrieves a specific mechanical sensor displacement/force metric by value.
 * \param[in] index The target \ref ASDriverMechanicalSensors sensor identifier.
 * \return The raw metrics value stored (mm / N).
 */
float as_driver_api_get_mechanical_sensor(enum ASDriverMechanicalSensor index);

/*!
 * \brief Retrieves a read-only pointer to the entire mechanical sensors array.
 * \return A constant pointer to the first element of an array containing 
 */
const float *as_driver_api_get_all_mechanical_sensors(void);

/*!
 * \brief Sets the command state of the Remote Emergency System (RES).
 * \param[in] signal The active \ref ASDriverRESSignal state code to apply.
 */
void as_driver_api_set_res_signal(enum ASDriverRESSignal signal);

/*!
 * \brief Gets the command state of the Remote Emergency System (RES).
 * \return The current active \ref ASDriverRESSignal state code.
 */
enum ASDriverRESSignal as_driver_api_get_res_signal(void);

/*!
 * \brief Sets the high-level evaluation state of the watchdog.
 * \param[in] state The \ref ASDriverWatchdogState tracking token to assign.
 */
void as_driver_api_set_watchdog_state(enum ASDriverWatchdogState state);

/*!
 * \brief Gets the high-level evaluation state of the watchdog.
 * \return The current registered \ref ASDriverWatchdogState tracking token.
 */
enum ASDriverWatchdogState as_driver_api_get_watchdog_state(void);

/*!
 * \brief Sets the physical Autonomous System Master Switch (ASMS) status flag.
 * \param[in] status True if the ASMS is switched on. False otherwise.
 */
void as_driver_api_set_asms_on(bool status);

/*!
 * \brief Gets the physical Autonomous System Master Switch (ASMS) status flag.
 * \return True if the ASMS is switched on. False otherwise.
 */
bool as_driver_api_get_asms_on(void);

/*!
 * \brief Sets the mission started status flag.
 * \param[in] status True if the autonomous mission run has begun. False otherwise.
 */
void as_driver_api_set_mission_started(bool status);

/*!
 * \brief Gets the mission started status flag.
 * \return True if the autonomous mission run has begun. False otherwise.
 */
bool as_driver_api_get_mission_started(void);

/*!
 * \brief Sets the physical Tractive System Master Switch (TSMS) status flag.
 * \param[in] status True if the TSMS is switched on.
 */
void as_driver_api_set_tsms_on(bool status);

/*!
 * \brief Gets the physical Tractive System Master Switch (TSMS) status flag.
 * \return True if the TSMS is switched on. False otherwise.
 */
bool as_driver_api_get_tsms_on(void);

/*!
 * \brief Sets the physical Shutdown Circuit (SDC) closed safety status flag.
 * \param[in] status True if the SDC is closed. False otherwise.
 */
void as_driver_api_set_sdc_closed(bool status);

/*!
 * \brief Gets the physical Shutdown Circuit (SDC) closed safety status flag.
 * \return True if the SDC is closed. False otherwise.
 */
bool as_driver_api_get_sdc_closed(void);

/*!
 * \brief Sets the Emergency Brake System (EBS) active/tripped flag.
 * \param[in] status Indicate whether the EBS has been activated or not.
 */
void as_driver_api_set_ebs_active(bool status);

/*!
 * \brief Gets the Emergency Brake System (EBS) active/tripped flag.
 * \return True if EBS has been activated. False otherwise.
 */
bool as_driver_api_get_ebs_active(void);

/*!
 * \brief Sets the vehicle standstill status flag.
 * \param[in] is_standstill True if the car is standstill. False otherwise.
 */
void as_driver_api_set_standstill(bool is_standstill);

/*!
 * \brief Gets the vehicle standstill status flag.
 * \return True if zero velocity is confirmed. False if the car is not standstill.
 */
bool as_driver_api_get_standstill(void);

#endif