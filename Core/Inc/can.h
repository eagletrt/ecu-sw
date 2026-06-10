/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    can.h
  * @brief   This file contains all the function prototypes for
  *          the can.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CAN_H__
#define __CAN_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include "as-driver.h"
#include "inverters.h"
#include "tractive-system.h"
/* USER CODE END Includes */

extern CAN_HandleTypeDef hcan1;

extern CAN_HandleTypeDef hcan2;

extern CAN_HandleTypeDef hcan3;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_CAN1_Init(void);
void MX_CAN2_Init(void);
void MX_CAN3_Init(void);

/* USER CODE BEGIN Prototypes */

/*!
 * \brief Transmits an air release command over the CAN bus.
 *
 * \param[in] air_line The targeted physical or logical AS air line variation to actuate.
 *
 * \retval AS_DRIVER_RC_OK if the CAN frame was successfully queued for transmission.
 * \retval AS_DRIVER_RC_ERROR if the frame could not be sent or actuation failed.
 */
enum ASDriverReturnCode can_air_release_from_line(enum ASDriverAirLine air_line);

/*!
 * \brief Dispatches a drive state command to the motor inverters.
 *
 * \param[in] drive_status The requested operational status target for the inverters.
 * \param[in] position The specific target inverter.
 *
 * \retval INVERTERS_RC_OK if the command was dispatched onto the network bus successfully.
 * \retval INVERTERS_RC_ERROR if the frame could not be sent.
 */
enum InvertersReturnCode can_inverters_send_drive_command(enum InvertersDriveStatus drive_status, enum InvertersPosition position);

/*!
 * \brief Modifies and transmits the commanded reference torque targets to the designated inverter.
 *
 * \param[in] target_torque The target physical torque reference value requested by the throttle pedal.
 * \param[in] position The specific target inverter.
 *
 * \retval INVERTERS_RC_OK if the torque setpoint update was successfully transmitted.
 * \retval INVERTERS_RC_ERROR if frame could not be sent or it was not possible to change the torque.
 */
enum InvertersReturnCode can_inverters_set_torque(float target_torque, enum InvertersPosition position);

/*!
 * \brief Transmits TS commands over the CAN bus.
 *
 * \param[in] ts_command The explicit Tractive System command state to broadcast.
 *
 * \retval TS_RC_OK if the frame was successfully transmitted.
 * \retval TS_RC_ERROR if the frame could not be sent.
 */
enum TSReturnCode can_ts_send_command(enum TSCommand ts_command);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __CAN_H__ */
