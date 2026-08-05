/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdio.h>

extern UART_HandleTypeDef huart2;

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

TaskHandle_t eventTaskHandle = NULL;

volatile uint32_t button_isr_cycle = 0U;
volatile uint32_t event_count = 0U;
volatile uint32_t notification_batch = 0U;

volatile uint32_t wake_latency_cycles = 0U;
volatile uint32_t wake_latency_us = 0U;

/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

static void EventTask(void *argument)
{
    (void)argument;

    char message[96];

    for (;;)
    {
        /*
         * Task 沒有事件時進入 Blocked 狀態。
         *
         * pdTRUE：
         * 讀取後把 Notification Count 清成 0。
         */
        uint32_t received_count =
            ulTaskNotifyTake(
                pdTRUE,
                portMAX_DELAY);

        uint32_t task_cycle = DWT->CYCCNT;

        wake_latency_cycles =
            task_cycle - button_isr_cycle;

        wake_latency_us =
            wake_latency_cycles /
            (SystemCoreClock / 1000000U);

        notification_batch = received_count;
        event_count += received_count;

        HAL_GPIO_TogglePin(
            GPIOA,
            GPIO_PIN_5);

        int length = snprintf(
            message,
            sizeof(message),
            "event=%lu, batch=%lu, latency=%lu us\r\n",
            (unsigned long)event_count,
            (unsigned long)notification_batch,
            (unsigned long)wake_latency_us);

        if (length > 0)
        {
            HAL_UART_Transmit(
                &huart2,
                (uint8_t *)message,
                (uint16_t)length,
                100U);
        }
    }
}

void App_FreeRTOS_Init(void)
{
    BaseType_t result = xTaskCreate(
        EventTask,
        "EventTask",
        256U,
        NULL,
        tskIDLE_PRIORITY + 3U,
        &eventTaskHandle);

    configASSERT(result == pdPASS);
    configASSERT(eventTaskHandle != NULL);
}

/* USER CODE END Application */

