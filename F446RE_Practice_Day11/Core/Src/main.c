/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/*
 * 0：使用 Binary Semaphore，沒有 Priority Inheritance
 * 1：使用 Mutex，啟用 Priority Inheritance
 */
#define USE_PRIORITY_INHERITANCE 0

#define BUTTON_EVENT_FLAG       (1UL << 0)
#define LOW_WORK_ITERATIONS     20000UL
#define MEDIUM_WORK_ITERATIONS  5000UL


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

volatile uint32_t event_set_result = 0U;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for HighTask */
osThreadId_t HighTaskHandle;
const osThreadAttr_t HighTask_attributes = {
  .name = "HighTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for MediumTask */
osThreadId_t MediumTaskHandle;
const osThreadAttr_t MediumTask_attributes = {
  .name = "MediumTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for LowTask */
osThreadId_t LowTaskHandle;
const osThreadAttr_t LowTask_attributes = {
  .name = "LowTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* USER CODE BEGIN PV */

osMutexId_t resourceMutexHandle;
osSemaphoreId_t resourceSemaphoreHandle;

volatile uint8_t low_holding_resource = 0U;

volatile uint32_t high_wait_ticks = 0U;
volatile uint32_t high_request_count = 0U;
volatile uint32_t work_sink = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
void StartDefaultTask(void *argument);
void StartHighTask(void *argument);
void StartMediumTask(void *argument);
void StartLowTask(void *argument);

/* USER CODE BEGIN PFP */

static osStatus_t ResourceLock(void);
static osStatus_t ResourceUnlock(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static osStatus_t ResourceLock(void)
{
#if USE_PRIORITY_INHERITANCE

    return osMutexAcquire(
        resourceMutexHandle,
        osWaitForever);

#else

    return osSemaphoreAcquire(
        resourceSemaphoreHandle,
        osWaitForever);

#endif
}

static osStatus_t ResourceUnlock(void)
{
#if USE_PRIORITY_INHERITANCE

    return osMutexRelease(
        resourceMutexHandle);

#else

    return osSemaphoreRelease(
        resourceSemaphoreHandle);

#endif
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

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
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  const char boot_message[] = "\r\n=== RTOS test boot ===\r\n";

  HAL_UART_Transmit(
      &huart2,
      (uint8_t *)boot_message,
      sizeof(boot_message) - 1U,
      HAL_MAX_DELAY);

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */

  const osMutexAttr_t resourceMutexAttributes =
  {
      .name = "resourceMutex",
      .attr_bits = osMutexPrioInherit,
      .cb_mem = NULL,
      .cb_size = 0U
  };

  resourceMutexHandle =
      osMutexNew(&resourceMutexAttributes);

  resourceSemaphoreHandle =
      osSemaphoreNew(
          1U,     /* 最大數量 */
          1U,     /* 初始數量 */
          NULL);


  if ((resourceMutexHandle == NULL) ||
      (resourceSemaphoreHandle == NULL))
  {
      Error_Handler();
  }

  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of HighTask */
  HighTaskHandle = osThreadNew(StartHighTask, NULL, &HighTask_attributes);

  /* creation of MediumTask */
  MediumTaskHandle = osThreadNew(StartMediumTask, NULL, &MediumTask_attributes);

  /* creation of LowTask */
  LowTaskHandle = osThreadNew(StartLowTask, NULL, &LowTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

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
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_LED_GPIO_Port, LD2_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_LED_Pin */
  GPIO_InitStruct.Pin = LD2_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_LED_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

volatile uint32_t button_isr_count = 0U;
volatile uint32_t thread_flag_result = 0U;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_13)
    {
        button_isr_count++;

        if ((HighTaskHandle != NULL) &&
            (osKernelGetState() == osKernelRunning))
        {
            thread_flag_result =
                osThreadFlagsSet(
                    HighTaskHandle,
                    BUTTON_EVENT_FLAG);
        }
    }
}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartHighTask */
/**
* @brief Function implementing the HighTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartHighTask */
void StartHighTask(void *argument)
{
  /* USER CODE BEGIN StartHighTask */
  /* Infinite loop */
	 (void)argument;

	  char message[96];

	  for (;;)
	  {
		  uint32_t flags = osThreadFlagsWait(
		      BUTTON_EVENT_FLAG,
		      osFlagsWaitAny,
		      osWaitForever);

	    if ((flags & osFlagsError) != 0U)
	    {
	      continue;
	    }

	    uint32_t start_tick =
	        osKernelGetTickCount();

	    if (ResourceLock() == osOK)
	    {
	      uint32_t end_tick =
	          osKernelGetTickCount();

	      high_wait_ticks =
	          end_tick - start_tick;

	      high_request_count++;

	      (void)ResourceUnlock();

	      uint32_t tick_frequency =
	          osKernelGetTickFreq();

	      uint32_t wait_ms = 0U;

	      if (tick_frequency != 0U)
	      {
	        wait_ms =
	            (high_wait_ticks * 1000U) /
	            tick_frequency;
	      }

	      int length = snprintf(
	          message,
	          sizeof(message),
	          "HighTask request=%lu, wait=%lu ms, mode=%s\r\n",
	          (unsigned long)high_request_count,
	          (unsigned long)wait_ms,
	#if USE_PRIORITY_INHERITANCE
	          "Mutex+Inheritance"
	#else
	          "Binary Semaphore"
	#endif
	      );

	      if (length > 0)
	      {
	        uint16_t transmit_length;

	        if ((size_t)length < sizeof(message))
	        {
	          transmit_length = (uint16_t)length;
	        }
	        else
	        {
	          transmit_length =
	              (uint16_t)(sizeof(message) - 1U);
	        }

	        (void)HAL_UART_Transmit(
	            &huart2,
	            (uint8_t *)message,
	            transmit_length,
	            100U);
	      }

	      /*
	       * 快閃三次代表 HighTask 已取得資源。
	       */
	      for (uint32_t i = 0U; i < 3U; i++)
	      {
	        HAL_GPIO_WritePin(
	            LD2_LED_GPIO_Port,
	            LD2_LED_Pin,
	            GPIO_PIN_SET);

	        osDelay(50U);

	        HAL_GPIO_WritePin(
	            LD2_LED_GPIO_Port,
	            LD2_LED_Pin,
	            GPIO_PIN_RESET);

	        osDelay(50U);
	      }
	    }
	  }
  /* USER CODE END StartHighTask */
}

/* USER CODE BEGIN Header_StartMediumTask */
/**
* @brief Function implementing the MediumTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartMediumTask */
void StartMediumTask(void *argument)
{
  /* USER CODE BEGIN StartMediumTask */
  /* Infinite loop */
	  (void)argument;

	  for (;;)
	  {
	    if (low_holding_resource != 0U)
	    {
	      for (uint32_t i = 0U;
	           i < MEDIUM_WORK_ITERATIONS;
	           i++)
	      {
	        work_sink += i;
	      }

	      /*
	       * 短暫讓出 CPU，避免完全餓死 LowTask。
	       */
	      osDelay(1U);
	    }
	    else
	    {
	      osDelay(1U);
	    }
	  }

  /* USER CODE END StartMediumTask */
}

/* USER CODE BEGIN Header_StartLowTask */
/**
* @brief Function implementing the LowTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLowTask */
void StartLowTask(void *argument)
{
  /* USER CODE BEGIN StartLowTask */
  /* Infinite loop */
	  (void)argument;

	  for (;;)
	  {
	    if (ResourceLock() == osOK)
	    {
	      low_holding_resource = 1U;

	      HAL_GPIO_WritePin(
	          LD2_LED_GPIO_Port,
	          LD2_LED_Pin,
	          GPIO_PIN_SET);

	      /*
	       * 模擬 LowTask 持有共用資源，
	       * 並進行需要 CPU 時間的工作。
	       */
	      for (uint32_t i = 0U;
	           i < LOW_WORK_ITERATIONS;
	           i++)
	      {
	        work_sink =
	            (work_sink * 1664525UL) +
	            1013904223UL;
	      }

	      low_holding_resource = 0U;

	      HAL_GPIO_WritePin(
	          LD2_LED_GPIO_Port,
	          LD2_LED_Pin,
	          GPIO_PIN_RESET);

	      (void)ResourceUnlock();
	    }

	    osDelay(1500U);
	  }

  /* USER CODE END StartLowTask */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
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
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
