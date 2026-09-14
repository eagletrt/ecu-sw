/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    can.c
  * @brief   This file provides code for the configuration
  *          of the CAN instances.
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
/* Includes ------------------------------------------------------------------*/
#include "can.h"

/* USER CODE BEGIN 0 */
#include "can-communication-api.h"
#include "eagletrt-api.h"
#include "logger-api.h"
#include "can-inverters.h"

/*!
 * \brief Upper bound (ms) spent waiting for a free bxCAN TX mailbox before giving up.
 * \details Only reached when all three mailboxes stay busy, which needs a genuinely
 *     stuck/off bus; a normal 1 Mbit/s frame clears a mailbox in ~130 us, so the wait
 *     loop almost always exits far sooner. Kept small so a stuck bus cannot stall the
 *     main loop for long.
 */
#define CAN_TX_MAILBOX_FREE_TIMEOUT_MS (2U)
/* USER CODE END 0 */

CAN_HandleTypeDef hcan1;
CAN_HandleTypeDef hcan2;
CAN_HandleTypeDef hcan3;

/* CAN1 init function */
void MX_CAN1_Init(void) {

    /* USER CODE BEGIN CAN1_Init 0 */

    /* USER CODE END CAN1_Init 0 */

    /* USER CODE BEGIN CAN1_Init 1 */

    /* USER CODE END CAN1_Init 1 */
    hcan1.Instance = CAN1;
    hcan1.Init.Prescaler = 2;
    hcan1.Init.Mode = CAN_MODE_NORMAL;
    hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan1.Init.TimeSeg1 = CAN_BS1_13TQ;
    hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;
    hcan1.Init.TimeTriggeredMode = DISABLE;
    hcan1.Init.AutoBusOff = DISABLE;
    hcan1.Init.AutoWakeUp = DISABLE;
    hcan1.Init.AutoRetransmission = DISABLE;
    hcan1.Init.ReceiveFifoLocked = DISABLE;
    hcan1.Init.TransmitFifoPriority = DISABLE;
    if (HAL_CAN_Init(&hcan1) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN CAN1_Init 2 */

    // odd IDs go to FIFO1, even IDs go to FIFO0
    HAL_CAN_ConfigFilter(&hcan1, &(CAN_FilterTypeDef){ .FilterBank = 0, .FilterMode = CAN_FILTERMODE_IDMASK, .FilterScale = CAN_FILTERSCALE_32BIT, .FilterIdHigh = 0x0000, .FilterIdLow = 0x0000, .FilterMaskIdHigh = 0x0020, .FilterMaskIdLow = 0x0000, .FilterFIFOAssignment = CAN_RX_FIFO0, .FilterActivation = ENABLE, .SlaveStartFilterBank = 14 });
    HAL_CAN_ConfigFilter(&hcan1, &(CAN_FilterTypeDef){ .FilterBank = 1, .FilterMode = CAN_FILTERMODE_IDMASK, .FilterScale = CAN_FILTERSCALE_32BIT, .FilterIdHigh = 0x0020, .FilterIdLow = 0x0000, .FilterMaskIdHigh = 0x0020, .FilterMaskIdLow = 0x0000, .FilterFIFOAssignment = CAN_RX_FIFO1, .FilterActivation = ENABLE, .SlaveStartFilterBank = 14 });

    /* USER CODE END CAN1_Init 2 */
}
/* CAN2 init function */
void MX_CAN2_Init(void) {

    /* USER CODE BEGIN CAN2_Init 0 */

    /* USER CODE END CAN2_Init 0 */

    /* USER CODE BEGIN CAN2_Init 1 */

    /* USER CODE END CAN2_Init 1 */
    hcan2.Instance = CAN2;
    hcan2.Init.Prescaler = 2;
    hcan2.Init.Mode = CAN_MODE_NORMAL;
    hcan2.Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan2.Init.TimeSeg1 = CAN_BS1_13TQ;
    hcan2.Init.TimeSeg2 = CAN_BS2_2TQ;
    hcan2.Init.TimeTriggeredMode = DISABLE;
    hcan2.Init.AutoBusOff = DISABLE;
    hcan2.Init.AutoWakeUp = DISABLE;
    hcan2.Init.AutoRetransmission = DISABLE;
    hcan2.Init.ReceiveFifoLocked = DISABLE;
    hcan2.Init.TransmitFifoPriority = DISABLE;
    if (HAL_CAN_Init(&hcan2) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN CAN2_Init 2 */

    // odd IDs go to FIFO1, even IDs go to FIFO0
    HAL_CAN_ConfigFilter(&hcan2, &(CAN_FilterTypeDef){ .FilterBank = 14, .FilterMode = CAN_FILTERMODE_IDMASK, .FilterScale = CAN_FILTERSCALE_32BIT, .FilterIdHigh = 0x0000, .FilterIdLow = 0x0000, .FilterMaskIdHigh = 0x0020, .FilterMaskIdLow = 0x0000, .FilterFIFOAssignment = CAN_RX_FIFO0, .FilterActivation = ENABLE, .SlaveStartFilterBank = 14 });
    HAL_CAN_ConfigFilter(&hcan2, &(CAN_FilterTypeDef){ .FilterBank = 15, .FilterMode = CAN_FILTERMODE_IDMASK, .FilterScale = CAN_FILTERSCALE_32BIT, .FilterIdHigh = 0x0020, .FilterIdLow = 0x0000, .FilterMaskIdHigh = 0x0020, .FilterMaskIdLow = 0x0000, .FilterFIFOAssignment = CAN_RX_FIFO1, .FilterActivation = ENABLE, .SlaveStartFilterBank = 14 });

    /* USER CODE END CAN2_Init 2 */
}
/* CAN3 init function */
void MX_CAN3_Init(void) {

    /* USER CODE BEGIN CAN3_Init 0 */

    /* USER CODE END CAN3_Init 0 */

    /* USER CODE BEGIN CAN3_Init 1 */

    /* USER CODE END CAN3_Init 1 */
    hcan3.Instance = CAN3;
    hcan3.Init.Prescaler = 2;
    hcan3.Init.Mode = CAN_MODE_NORMAL;
    hcan3.Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan3.Init.TimeSeg1 = CAN_BS1_13TQ;
    hcan3.Init.TimeSeg2 = CAN_BS2_2TQ;
    hcan3.Init.TimeTriggeredMode = DISABLE;
    hcan3.Init.AutoBusOff = DISABLE;
    hcan3.Init.AutoWakeUp = DISABLE;
    hcan3.Init.AutoRetransmission = DISABLE;
    hcan3.Init.ReceiveFifoLocked = DISABLE;
    hcan3.Init.TransmitFifoPriority = DISABLE;
    if (HAL_CAN_Init(&hcan3) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN CAN3_Init 2 */

    // odd IDs go to FIFO1, even IDs go to FIFO0
    HAL_CAN_ConfigFilter(&hcan3, &(CAN_FilterTypeDef){ .FilterBank = 0, .FilterMode = CAN_FILTERMODE_IDMASK, .FilterScale = CAN_FILTERSCALE_32BIT, .FilterIdHigh = 0x0000, .FilterIdLow = 0x0000, .FilterMaskIdHigh = 0x0020, .FilterMaskIdLow = 0x0000, .FilterFIFOAssignment = CAN_RX_FIFO0, .FilterActivation = ENABLE, .SlaveStartFilterBank = 14 });
    HAL_CAN_ConfigFilter(&hcan3, &(CAN_FilterTypeDef){ .FilterBank = 1, .FilterMode = CAN_FILTERMODE_IDMASK, .FilterScale = CAN_FILTERSCALE_32BIT, .FilterIdHigh = 0x0020, .FilterIdLow = 0x0000, .FilterMaskIdHigh = 0x0020, .FilterMaskIdLow = 0x0000, .FilterFIFOAssignment = CAN_RX_FIFO1, .FilterActivation = ENABLE, .SlaveStartFilterBank = 14 });

    /* USER CODE END CAN3_Init 2 */
}

static uint32_t HAL_RCC_CAN1_CLK_ENABLED = 0;

void HAL_CAN_MspInit(CAN_HandleTypeDef *canHandle) {

    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    if (canHandle->Instance == CAN1) {
        /* USER CODE BEGIN CAN1_MspInit 0 */

        /* USER CODE END CAN1_MspInit 0 */
        /* CAN1 clock enable */
        HAL_RCC_CAN1_CLK_ENABLED++;
        if (HAL_RCC_CAN1_CLK_ENABLED == 1) {
            __HAL_RCC_CAN1_CLK_ENABLE();
        }

        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**CAN1 GPIO Configuration
    PA11     ------> CAN1_RX
    PA12     ------> CAN1_TX
    */
        GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* CAN1 interrupt Init */
        HAL_NVIC_SetPriority(CAN1_TX_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(CAN1_TX_IRQn);
        HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
        HAL_NVIC_SetPriority(CAN1_RX1_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(CAN1_RX1_IRQn);
        /* USER CODE BEGIN CAN1_MspInit 1 */

        /* USER CODE END CAN1_MspInit 1 */
    } else if (canHandle->Instance == CAN2) {
        /* USER CODE BEGIN CAN2_MspInit 0 */

        /* USER CODE END CAN2_MspInit 0 */
        /* CAN2 clock enable */
        __HAL_RCC_CAN2_CLK_ENABLE();
        HAL_RCC_CAN1_CLK_ENABLED++;
        if (HAL_RCC_CAN1_CLK_ENABLED == 1) {
            __HAL_RCC_CAN1_CLK_ENABLE();
        }

        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**CAN2 GPIO Configuration
    PB12     ------> CAN2_RX
    PB13     ------> CAN2_TX
    */
        GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF9_CAN2;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* CAN2 interrupt Init */
        HAL_NVIC_SetPriority(CAN2_TX_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(CAN2_TX_IRQn);
        HAL_NVIC_SetPriority(CAN2_RX0_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(CAN2_RX0_IRQn);
        HAL_NVIC_SetPriority(CAN2_RX1_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(CAN2_RX1_IRQn);
        /* USER CODE BEGIN CAN2_MspInit 1 */

        /* USER CODE END CAN2_MspInit 1 */
    } else if (canHandle->Instance == CAN3) {
        /* USER CODE BEGIN CAN3_MspInit 0 */

        /* USER CODE END CAN3_MspInit 0 */
        /* CAN3 clock enable */
        __HAL_RCC_CAN3_CLK_ENABLE();

        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**CAN3 GPIO Configuration
    PA8     ------> CAN3_RX
    PA15     ------> CAN3_TX
    */
        GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_15;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF11_CAN3;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* CAN3 interrupt Init */
        HAL_NVIC_SetPriority(CAN3_TX_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(CAN3_TX_IRQn);
        HAL_NVIC_SetPriority(CAN3_RX0_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(CAN3_RX0_IRQn);
        HAL_NVIC_SetPriority(CAN3_RX1_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(CAN3_RX1_IRQn);
        /* USER CODE BEGIN CAN3_MspInit 1 */

        /* USER CODE END CAN3_MspInit 1 */
    }
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef *canHandle) {

    if (canHandle->Instance == CAN1) {
        /* USER CODE BEGIN CAN1_MspDeInit 0 */

        /* USER CODE END CAN1_MspDeInit 0 */
        /* Peripheral clock disable */
        HAL_RCC_CAN1_CLK_ENABLED--;
        if (HAL_RCC_CAN1_CLK_ENABLED == 0) {
            __HAL_RCC_CAN1_CLK_DISABLE();
        }

        /**CAN1 GPIO Configuration
    PA11     ------> CAN1_RX
    PA12     ------> CAN1_TX
    */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11 | GPIO_PIN_12);

        /* CAN1 interrupt Deinit */
        HAL_NVIC_DisableIRQ(CAN1_TX_IRQn);
        HAL_NVIC_DisableIRQ(CAN1_RX0_IRQn);
        HAL_NVIC_DisableIRQ(CAN1_RX1_IRQn);
        /* USER CODE BEGIN CAN1_MspDeInit 1 */

        /* USER CODE END CAN1_MspDeInit 1 */
    } else if (canHandle->Instance == CAN2) {
        /* USER CODE BEGIN CAN2_MspDeInit 0 */

        /* USER CODE END CAN2_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_CAN2_CLK_DISABLE();
        HAL_RCC_CAN1_CLK_ENABLED--;
        if (HAL_RCC_CAN1_CLK_ENABLED == 0) {
            __HAL_RCC_CAN1_CLK_DISABLE();
        }

        /**CAN2 GPIO Configuration
    PB12     ------> CAN2_RX
    PB13     ------> CAN2_TX
    */
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_12 | GPIO_PIN_13);

        /* CAN2 interrupt Deinit */
        HAL_NVIC_DisableIRQ(CAN2_TX_IRQn);
        HAL_NVIC_DisableIRQ(CAN2_RX0_IRQn);
        HAL_NVIC_DisableIRQ(CAN2_RX1_IRQn);
        /* USER CODE BEGIN CAN2_MspDeInit 1 */

        /* USER CODE END CAN2_MspDeInit 1 */
    } else if (canHandle->Instance == CAN3) {
        /* USER CODE BEGIN CAN3_MspDeInit 0 */

        /* USER CODE END CAN3_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_CAN3_CLK_DISABLE();

        /**CAN3 GPIO Configuration
    PA8     ------> CAN3_RX
    PA15     ------> CAN3_TX
    */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_8 | GPIO_PIN_15);

        /* CAN3 interrupt Deinit */
        HAL_NVIC_DisableIRQ(CAN3_TX_IRQn);
        HAL_NVIC_DisableIRQ(CAN3_RX0_IRQn);
        HAL_NVIC_DisableIRQ(CAN3_RX1_IRQn);
        /* USER CODE BEGIN CAN3_MspDeInit 1 */

        /* USER CODE END CAN3_MspDeInit 1 */
    }
}

/* USER CODE BEGIN 1 */
/*!
 * \brief Returns the CAN network used based on the native ST HAL CAN handle.
 * \warning If the \c hcan refers to an undefined instance, the returned value is a non-valid network (\c CAN_COMMUNICATION_NETWORK_COUNT).
 * \param[in] hcan Pointer to the ST HAL CAN handle instance structure.
 * \retval CAN_COMMUNICATION_NETWORK_PRIMARY If the handler refers to the primary network peripheral instance.
 * \retval CAN_COMMUNICATION_NETWORK_SECONDARY If the handler refers to the secondary network peripheral instance.
 * \retval CAN_COMMUNICATION_NETWORK_INVERTER If the handler refers to the inverter network peripheral instance.
 * \retval CAN_COMMUNICATION_NETWORK_COUNT If the handler doesn't refer to any valid CAN network.
 */
EAGLETRT_STATIC_INLINE enum CanCommunicationNetwork prv_can_get_network(const CAN_HandleTypeDef *hcan) {
    if (hcan == NULL || hcan->Instance == NULL) {
        return CAN_COMMUNICATION_NETWORK_COUNT;
    }

    if (hcan->Instance == CAN1) {
        return CAN_COMMUNICATION_NETWORK_PRIMARY;
    }

    if (hcan->Instance == CAN2) {
        return CAN_COMMUNICATION_NETWORK_SECONDARY;
    }

    if (hcan->Instance == CAN3) {
        return CAN_COMMUNICATION_NETWORK_INVERTER;
    }

    return CAN_COMMUNICATION_NETWORK_COUNT;
}

/*!
 * \brief Returns the native ST HAL CAN handler based on the network enum.
 * \param[in] network The target network track enum.
 * \return Pointer to the matched global CAN_HandleTypeDef, or \c NULL if invalid.
 */
EAGLETRT_STATIC_INLINE CAN_HandleTypeDef *prv_can_get_handler(enum CanCommunicationNetwork network) {
    switch (network) {
        case CAN_COMMUNICATION_NETWORK_PRIMARY:
            return &hcan1;
        case CAN_COMMUNICATION_NETWORK_SECONDARY:
            return &hcan2;
        case CAN_COMMUNICATION_NETWORK_INVERTER:
            return &hcan3;
        default:
            return NULL;
    }
}

/*!
 * \brief Internal unified helper to write an abstract frame out to an ST HAL CAN peripheral.
 * \param[in] network The network track enum indicating which hardware peripheral to target.
 * \param[in] frame Pointer to the abstract frame structure containing the payload.
 *
 * \retval CAN_COMMUNICATION_RC_OK if the frame was sent successfully.
 * \retval CAN_COMMUNICATION_RC_NULL_POINTER if a required pointer configuration is \c NULL.
 * \retval CAN_COMMUNICATION_RC_INVALID_LENGTH if the frame length exceeds CAN_COMMUNICATION_FRAME_DATA_SIZE.
 * \retval CAN_COMMUNICATION_RC_TRANSMISSION_ERROR if the native HAL layer rejects the transmission.
 */
EAGLETRT_STATIC enum CanCommunicationReturnCode prv_can_send_to_hardware(enum CanCommunicationNetwork network, const struct CanCommunicationFrame *frame) {
    CAN_HandleTypeDef *hcan = prv_can_get_handler(network);

    if (hcan == NULL || frame == NULL) {
        return CAN_COMMUNICATION_RC_NULL_POINTER;
    }
    if (frame->length > CAN_COMMUNICATION_FRAME_DATA_SIZE) {
        return CAN_COMMUNICATION_RC_INVALID_LENGTH;
    }

    uint32_t wait_start_tick = HAL_GetTick();
    while (HAL_CAN_GetTxMailboxesFreeLevel(hcan) == 0U) {
        if ((HAL_GetTick() - wait_start_tick) >= CAN_TX_MAILBOX_FREE_TIMEOUT_MS) {
            return CAN_COMMUNICATION_RC_TRANSMISSION_ERROR;
        }
    }

    CAN_TxHeaderTypeDef tx_header;
    uint32_t tx_mailbox = 0U;

    tx_header.StdId = frame->id;
    tx_header.ExtId = 0U;
    tx_header.IDE = CAN_ID_STD;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.DLC = frame->length;
    tx_header.TransmitGlobalTime = DISABLE;

    HAL_StatusTypeDef res = HAL_CAN_AddTxMessage(hcan, &tx_header, (uint8_t *)frame->data, &tx_mailbox);
    if (res != HAL_OK) {
        return CAN_COMMUNICATION_RC_TRANSMISSION_ERROR;
    }

    return CAN_COMMUNICATION_RC_OK;
}

enum CanCommunicationReturnCode can_send_primary(const struct CanCommunicationFrame *frame) {
    return prv_can_send_to_hardware(CAN_COMMUNICATION_NETWORK_PRIMARY, frame);
}

enum CanCommunicationReturnCode can_send_secondary(const struct CanCommunicationFrame *frame) {
    return prv_can_send_to_hardware(CAN_COMMUNICATION_NETWORK_SECONDARY, frame);
}

enum CanCommunicationReturnCode can_send_inverter(const struct CanCommunicationFrame *frame) {
    return prv_can_send_to_hardware(CAN_COMMUNICATION_NETWORK_INVERTER, frame);
}

enum ASDriverReturnCode can_air_release_from_line(enum ASDriverAirLine air_line) {
    EAGLETRT_API_UNUSED(air_line);
    return AS_DRIVER_RC_OK;
}

/*!
 * \brief Per-network count of hardware RX-FIFO overruns (frames the peripheral
 *     dropped because the FIFO was full when a new one arrived).
 * \details Purely observational: a value that keeps growing is direct evidence
 *     that the ISR is not draining fast enough (or the bus bursts harder than the
 *     6 buffered frames tolerate) - i.e. the "FIFO too shallow" failure. If this
 *     stays at zero while frames still go missing, the losses are NOT overrun
 *     (look at the bus/physical layer instead). Read it with
 *     \ref can_get_rx_overrun_count. Marked volatile: written in ISR, read in main.
 */
EAGLETRT_STATIC volatile uint32_t prv_can_rx_overrun_count[CAN_COMMUNICATION_NETWORK_COUNT] = { 0 };

uint32_t can_get_rx_overrun_count(enum CanCommunicationNetwork network) {
    if (network >= CAN_COMMUNICATION_NETWORK_COUNT) {
        return 0U;
    }
    return prv_can_rx_overrun_count[network];
}

/*!
 * \brief Drains every pending frame from one RX FIFO and enqueues them for processing.
 * \details Reads in a loop until the FIFO fill level reaches zero, so a burst that
 *     lands several frames before the ISR runs is emptied in a single interrupt
 *     entry instead of one frame per interrupt - the strongest guard against
 *     overrun. It also detects and clears the FIFO overrun flag so a genuine
 *     hardware drop is counted rather than lost silently.
 * \param[in] hcan Pointer to the ST HAL CAN handle that raised the interrupt.
 * \param[in] rx_fifo Either \c CAN_RX_FIFO0 or \c CAN_RX_FIFO1.
 */
EAGLETRT_STATIC void prv_can_drain_rx_fifo(CAN_HandleTypeDef *hcan, uint32_t rx_fifo) {
    enum CanCommunicationNetwork network = prv_can_get_network(hcan);
    if (network >= CAN_COMMUNICATION_NETWORK_COUNT) {
        return;
    }

    // A set overrun flag means the hardware already dropped at least one frame
    // because this FIFO was full. Count it for diagnostics, then clear it.
    const uint32_t overrun_flag = (rx_fifo == CAN_RX_FIFO0) ? CAN_FLAG_FOV0 : CAN_FLAG_FOV1;
    if (__HAL_CAN_GET_FLAG(hcan, overrun_flag) != 0U) {
        prv_can_rx_overrun_count[network]++;
        __HAL_CAN_CLEAR_FLAG(hcan, overrun_flag);
    }

    // Empty the whole FIFO in this single interrupt entry (its 3-frame depth plus
    // anything that arrives while we drain) so bursts do not pile up.
    while (HAL_CAN_GetRxFifoFillLevel(hcan, rx_fifo) > 0U) {
        CAN_RxHeaderTypeDef header = { 0 };
        struct CanCommunicationFrame msg = { 0 };

        if (HAL_CAN_GetRxMessage(hcan, rx_fifo, &header, msg.data) != HAL_OK) {
            break;
        }
        msg.id = (header.IDE == CAN_ID_EXT) ? header.ExtId : header.StdId;
        msg.length = (uint8_t)header.DLC;

        EAGLETRT_API_UNUSED(can_communication_api_add_to_rx(network, &msg));
    }
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    prv_can_drain_rx_fifo(hcan, CAN_RX_FIFO0);
    HAL_CAN_ActivateNotification(hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    prv_can_drain_rx_fifo(hcan, CAN_RX_FIFO1);
    HAL_CAN_ActivateNotification(hcan, CAN_IT_RX_FIFO1_MSG_PENDING);
}
/* USER CODE END 1 */
