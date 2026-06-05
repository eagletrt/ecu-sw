/*!
 * \file as-driver.h
 * \author Dorijan Di Zepp
 * \date 2026-06-05
 * \brief Callback and struct definitions for the Autonomous System (AS) driver.
 * \details This module encapsulates the state variables, sensor values
 * and system-level status flags necessary to govern the autonomous mission.
 */
#ifndef AS_DRIVER_H
#define AS_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

/*!
 * \brief Return codes for the AS driver functions and hardware callbacks.
 */
enum ASDriverReturnCode {
    AS_DRIVER_RC_OK,    /*!< Execution completed successfully. */
    AS_DRIVER_RC_ERROR, /*!< General execution or validation error. */
};

/*!
 * \brief Autonomous driving disciplines defined by the FS rulebook.
 */
enum ASDriverMission {
    AS_DRIVER_MISSION_NOT_SELECTED, /*!< Default uninitialized state. Vehicle must not actuate. */
    AS_DRIVER_MISSION_ACCELERATION, /*!< Straight line acceleration event. */
    AS_DRIVER_MISSION_SKIDPAD,      /*!< Figure-eight driving track layout event. */
    AS_DRIVER_MISSION_AUTOCROSS,    /*!< Single lap track layout event. */
    AS_DRIVER_MISSION_TRACKDRIVE,   /*!< Multi-lap endurance track layout event. */
    AS_DRIVER_MISSION_EBS_TEST,     /*!< Dedicated validation protocol for emergency braking. */
    AS_DRIVER_MISSION_INSPECTION,   /*!< Static system check and technical inspection mode. */
    AS_DRIVER_MISSION_COUNT,        /*!< Sentinel value used for input validation */
};

/*!
 * \brief Distinct air line channels regulating the Emergency Brake System (EBS).
 */
enum ASDriverAirLine {
    AS_DRIVER_AIR_LINE_1, /*!< Primary pneumatic actuation circuit channel. */
    AS_DRIVER_AIR_LINE_2, /*!< Secondary pneumatic actuation circuit channel. */
};

/*!
 * \brief Callback signature to handle mechanical/pneumatic venting.
 * \param[in] line Target pneumatic circuit channel to trigger.
 * \retval AS_DRIVER_RC_OK if actuation is deployed.
 * \retval AS_DRIVER_RC_ERROR if actuation failed.
 */
typedef enum ASDriverReturnCode (*air_release_from_line_callback)(enum ASDriverAirLine);

/*!
 * \brief Braking system pressure profiles spanning both pneumatic and hydraulic circuits.
 * \note Nomenclature and numbering attributes directly match the ASF (Autonomous System Form) 
 * architecture documentation. Used directly as indices for the internal tracking arrays.
 */
enum ASDriverBrakePressure {
    AS_DRIVER_BRAKE_PRESSURE_1_1,  /*!< Index for pneumatic supply circuit pressure point 1.1 (bar). */
    AS_DRIVER_BRAKE_PRESSURE_1_2,  /*!< Index for pneumatic supply circuit pressure point 1.2 (bar). */
    AS_DRIVER_BRAKE_PRESSURE_1_3,  /*!< Index for pneumatic supply circuit pressure point 1.3 (bar). */
    AS_DRIVER_BRAKE_PRESSURE_1_4,  /*!< Index for pneumatic supply circuit pressure point 1.4 (bar). */
    AS_DRIVER_BRAKE_PRESSURE_2_1,  /*!< Index for hydraulic brake caliper line pressure point 2.1 (Front) (bar). */
    AS_DRIVER_BRAKE_PRESSURE_2_2,  /*!< Index for hydraulic brake caliper line pressure point 2.2 (Rear) (bar). */
    AS_DRIVER_BRAKE_PRESSURE_COUNT /*!< Total number of mapped brake pressure sensor points. */
};

/*!
 * \brief Structural and mechanical actuator verification sensors.
 * \note Nomenclature and numbering attributes directly match the ASF (Autonomous System Form) 
 * architecture documentation. Used directly as indices for the internal tracking arrays.
 */
enum ASDriverMechanicalSensor {
    AS_DRIVER_MECHANICAL_SENSOR_BREMSWEG_1_1,   /*!< Index for linear travel feedback displacement sensor 1.1 (mm). */
    AS_DRIVER_MECHANICAL_SENSOR_BREMSWEG_1_2,   /*!< Index for linear travel feedback displacement sensor 1.2 (mm). */
    AS_DRIVER_MECHANICAL_SENSOR_BREMSKRAFT_3_1, /*!< Index for actuator force or structural load cell strain gauge (N). */
    AS_DRIVER_MECHANICAL_SENSOR_COUNT           /*!< Total number of mapped mechanical validation sensors. */
};

/*!
 * \brief Operational states of the Remote Emergency System (RES) interface.
 * \details Consolidates individual booleans into a single mutual token.
 */
enum ASDriverRESSignal {
    AS_DRIVER_RES_SIGNAL_NONE,      /*!< No active commands or packets received from the RES station. */
    AS_DRIVER_RES_SIGNAL_GO,        /*!< Remote Emergency System active execution 'GO' command packet received. */
    AS_DRIVER_RES_SIGNAL_EMERGENCY, /*!< Remote Emergency System has explicitly commanded an immediate emergency shutdown. */
    AS_DRIVER_RES_SIGNAL_COUNT      /*!< Sentinel value used for input validation */
};

/*!
 * \brief Internal evaluation states of the hardware watchdog check sequence.
 * \details Replaces concurrent booleans to enforce a safe, progressive verification tracking path.
 */
enum ASDriverWatchdogState {
    AS_DRIVER_WATCHDOG_STATE_UNTESTED, /*!< Default state indicating the startup watchdog test sequence has not yet executed. */
    AS_DRIVER_WATCHDOG_STATE_CHECKING, /*!< Watchdog validation pulse check is actively in progress. */
    AS_DRIVER_WATCHDOG_STATE_VERIFIED, /*!< Watchdog loop confirmed operational and working within prescribed runtime limits. */
    AS_DRIVER_WATCHDOG_STATE_FAILED,   /*!< Watchdog failure detected during test cycle. */
    AS_DRIVER_WATCHDOG_STATE_COUNT     /*!< Sentinel value used for input validation */
};

/*!
 * \brief Core AS driver handler.
 */
struct ASDriverHandler {
    enum ASDriverMission as_mission; /*!< Current active driverless mission. */

    float brake_pressures[AS_DRIVER_BRAKE_PRESSURE_COUNT];       /*!< Metrics for system pressures (bar), indexed by \ref ASDriverBrakePressure. */
    float mechanical_sensors[AS_DRIVER_MECHANICAL_SENSOR_COUNT]; /*!< Mechanical feedback data (mm / N), indexed by \ref ASDriverMechanicalSensors. */

    air_release_from_line_callback release_air; /*!< Callback to command pneumatic emergency relief valves. */

    enum ASDriverRESSignal res_signal;         /*!< Unified state tracker managing command signals from the RES. */
    enum ASDriverWatchdogState watchdog_state; /*!< Safety tracker status mapping the watchdog test. */

    bool is_asms_on;         /*!< Status of the Autonomous System Master Switch. */
    bool is_mission_started; /*!< Asserts if the vehicle is ready and is starting the mission. */

    bool is_tsms_on;    /*!< Status of the Tractive System Master Switch. */
    bool is_sdc_closed; /*!< Monitored state of the shutdown circuit line. */

    bool is_ebs_active;         /*!< Emergency Brake System tripped indicating forced pneumatic venting deployment. */
    bool is_vehicle_standstill; /*!< Evaluated vehicle dynamics indicator derived from wheel nodes to trigger \ref AS_FINISHED status. */
};

#endif