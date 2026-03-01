/*!
 * \file tractive-system.h
 * \author Dorijan Di Zepp
 * \date 01-03-2026
 * \brief Logic module for managing the Tractive System (TS) handler
 */

#ifndef TRACTIVE_SYSTEM_H
#define TRACTIVE_SYSTEM_H

/*!
 * \brief Return codes for the tractive system module APIs.
 */
enum TSReturnCode {
    TS_RC_OK,   /*!< Operation completed successfully */
    TS_RC_ERROR /*!< Operation NOT completed successfully */
};

/*!
 * \brief TS commands.
 */
enum TSCommand {
    TS_COMMAND_ON, /*!< Send a logical ON */
    TS_COMMAND_OFF /*!< Send a logical OFF */
};

/*!
 * \brief Physical hardware status reported by the TS controllers.
 * \details This enum is the return type for the feedback callback. 
 * It represents the real state of the hardware, allowing the FSM to 
 * verify that its commands were actually executed.
 * \note The logical state of the vehicle is maintained 
 * exclusively by the FSM.
 */
enum TsStatus {
    TS_STATUS_UNKNOWN,
    TS_STATUS_OFF,
    TS_STATUS_PRECHARGE,
    TS_STATUS_ON,
    TS_STATUS_FATAL
};

/*!
 * \brief Signature for the hardware command callback.
 * \param command The logical command (ON/OFF) to be executed by the hardware.
 * \return TS_RC_OK if the hardware command was sent successfully, TS_RC_ERROR otherwise.
 */
typedef enum TSReturnCode (*ts_command_callback)(enum TSCommand);

/*!
 * \brief Signature for the hardware feedback callback.
 * \return The actual physical status reported by the TS.
 */
typedef enum TsStatus (*ts_feedback_callback)(void);

/*!
 * \brief Internal state of the tractive system module.
 */
struct TsHandler {
    /*!
     * \brief Hardware callback to trigger physical state changes 
     */
    ts_command_callback send_ts_command;

    /*!
     * \brief Hardware callback to retrieve the current state of the TS
     */
    ts_feedback_callback ts_status;
};

#endif