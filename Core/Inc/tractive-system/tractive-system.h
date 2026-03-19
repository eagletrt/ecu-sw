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
 * \brief Signature for the hardware command callback.
 * \param command The logical command (ON/OFF) to be executed by the hardware.
 * \return TS_RC_OK if the hardware command was sent successfully, TS_RC_ERROR otherwise.
 */
typedef enum TSReturnCode (*ts_command_callback)(enum TSCommand);

/*!
 * \brief Internal state of the tractive system module.
 */
struct TsHandler {
    /*!
     * \brief Hardware callback to trigger physical state changes 
     */
    ts_command_callback send_ts_command;
};

#endif