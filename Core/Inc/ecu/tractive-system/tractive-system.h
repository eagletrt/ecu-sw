/*!
 * \file tractive-system.h
 * \author Dorijan Di Zepp
 * \date 2026-03-22
 * \brief Logic module for managing the Tractive System (TS)
 * \note It decouples the logical state machine (Precharge, Discharge, R2D) 
 * from the physical hardware implementation via a callback mechanism.
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
 * \param[in] command The logical command (ON/OFF) to be executed by the hardware.
 * \retval TS_RC_OK if the hardware command was sent successfully
 * \retval TS_RC_ERROR if it was not possible to send the hardware command.
 */
typedef enum TSReturnCode (*ts_command_callback)(enum TSCommand);

/*!
 * \brief Internal state of the tractive system module.
 */
struct TsHandler {
    ts_command_callback send_ts_command; /*!< Hardware callback to trigger physical state changes */
};

#endif