/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "can.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "arena-allocator-api.h"
#include "ecu_fsm.h"
#include "eagletrt-api.h"
#include "as-driver-api.h"
#include "buzzer-api.h"
#include "can-communication-api.h"
#include "can-communication-router-api.h"
#include "inverters-api.h"
#include "logger-api.h"
#include "pedals-api.h"
#include "post-api.h"
#include "raspberry-api.h"
#include "tractive-system-api.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
//TODO: Would be better to move the defines into a configuration file (e.g. ecu-config.h)
#define LOGGER_ENABLED (true)          /*!< Logger status: true to enable active logging, false to mute entirely. */
#define LOGGER_RX_CAPACITY (0U)        /*!< Receive queue depth. Set to 0 because the logger is transmit-only. */
#define LOGGER_TX_CAPACITY (10U)       /*!< Maximum number of log message packets allowed to sit in the outbound transmission queue. */
#define LOGGER_UART_MAX_MSG_SIZE (64U) /*!< Maximum allocation allowed for an individual log string. */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
state_t current_state = STATE_INIT;

EAGLETRT_STATIC struct ArenaAllocatorHandler arena_allocator_handler;
EAGLETRT_STATIC struct PalHandler logger_pal_handler;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
EAGLETRT_STATIC void VectorBase_Config(void) {
    /* The constant array with vectors of the vector table is declared externally in the
  * c-startup code.
  */
    extern const unsigned long g_pfnVectors[];

    /* Remap the vector table to where the vector table is located for this program. */
    SCB->VTOR = (unsigned long)&g_pfnVectors[0];
}

/*!
 * \brief Initializes the low-level memory allocation and logging framework.
 */
EAGLETRT_STATIC void prv_main_init_logging_configuration() {
    arena_allocator_api_init(&arena_allocator_handler);

    EAGLETRT_API_UNUSED(pal_api_init(&logger_pal_handler,
                                     LOGGER_RX_CAPACITY,
                                     LOGGER_TX_CAPACITY,
                                     LOGGER_UART_MAX_MSG_SIZE,
                                     NULL,
                                     usart_logger_transmit,
                                     NULL,
                                     NULL,
                                     &arena_allocator_handler));
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void) {

    /* USER CODE BEGIN 1 */
    VectorBase_Config();
    /* USER CODE END 1 */

    /* MPU Configuration--------------------------------------------------------*/
    MPU_Config();

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_CAN1_Init();
    MX_CAN2_Init();
    MX_CAN3_Init();
    MX_UART4_Init();
    MX_TIM2_Init();
    MX_SPI2_Init();
    MX_SPI3_Init();
    /* USER CODE BEGIN 2 */

    // Initialize LOGGER configuration --------------------------------------------------
    // If logger fails to initialize, keep going as it shouldn't compromise the behavior of the entire car
    prv_main_init_logging_configuration();
    EAGLETRT_API_UNUSED(logger_api_init(&logger_pal_handler, LOGGER_ENABLED));
    // end of LOGGER configuration ------------------------------------------------------

    // Initialize POST configuration ----------------------------------------------------
    logger_api_log(LOGGER_LEVEL_DEBUG, "Initialize POST");

    // Buzzer configuration ----------------------------------------------------
    buzzer_on_callback buzzer_ons[BUZZER_TYPE_COUNT] = { gpio_buzzer_on, tim_buzzer_on };
    buzzer_off_callback buzzer_offs[BUZZER_TYPE_COUNT] = { gpio_buzzer_off, tim_buzzer_off };
    buzzer_delay_callback buzzer_syncs[BUZZER_TYPE_COUNT] = { gpio_buzzer_play_sync, tim_buzzer_play_sync };
    buzzer_tick_callback buzzer_ticks[BUZZER_TYPE_COUNT] = { HAL_GetTick, HAL_GetTick };

    // --- CAN communication configuration ---
    struct PostConfig post_configuration = {
        .can_networks = {
            [CAN_COMMUNICATION_NET_PRIMARY] = {
                .send = can_send_primary,
                .on_receive = can_communication_router_api_receive_primary,
                .cs_enter = nullptr,
                .cs_exit = nullptr,
            },
            [CAN_COMMUNICATION_NET_SECONDARY] = {
                .send = can_send_secondary,
                .on_receive = can_communication_router_api_receive_secondary,
                .cs_enter = nullptr,
                .cs_exit = nullptr,
            },
            [CAN_COMMUNICATION_NET_INVERTER] = {
                .send = can_send_inverter,
                .on_receive = can_communication_router_api_receive_inverter,
                .cs_enter = nullptr,
                .cs_exit = nullptr,
            } },

        // Direct assignment of single members during initialization
        .as_air_release = can_air_release_from_line,
        .inverters_send_drive_command = can_inverters_send_drive_command,
        .inverters_set_torque = can_inverters_set_torque,
        .raspberry_pin_control = gpio_raspberry_set_pin,
        .raspberry_initial_state = RASPBERRY_CONTROL_PIN_STATE_ON,
        .ts_send_command = can_ts_send_command
    };

    // Populate buzzer configuration arrays
    for (size_t i = 0; i < (size_t)BUZZER_TYPE_COUNT; i++) {
        post_configuration.buzzer_on_ptrs[i] = buzzer_ons[i];
        post_configuration.buzzer_off_ptrs[i] = buzzer_offs[i];
        post_configuration.buzzer_delay_ptrs[i] = buzzer_syncs[i];
        post_configuration.buzzer_tick_ptrs[i] = buzzer_ticks[i];
    }
    // end of POST configuration --------------------------------------------------------

    // single call run_state to verify POST
    current_state = run_state(current_state, &post_configuration);
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */

        //run the fsm
        current_state = run_state(current_state, NULL);
    }
    /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

    /** Configure the main internal regulator output voltage
  */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

    /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 128;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
  */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* MPU Configuration */

void MPU_Config(void) {
    MPU_Region_InitTypeDef MPU_InitStruct = { 0 };

    /* Disables the MPU */
    HAL_MPU_Disable();

    /** Initializes and configures the Region and the memory to be protected
  */
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER0;
    MPU_InitStruct.BaseAddress = 0x0;
    MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
    MPU_InitStruct.SubRegionDisable = 0x87;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
    MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    /* Enables the MPU */
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void) {
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line) {
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
