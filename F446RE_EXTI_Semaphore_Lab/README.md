# STM32F446RE — EXTI to FreeRTOS Binary Semaphore Lab

## 1. Objective

Implement event-driven synchronization between a GPIO external interrupt (EXTI) and a FreeRTOS task using a binary semaphore.

Pressing the B1 user button generates an interrupt. The interrupt callback releases a semaphore, allowing a blocked FreeRTOS task to wake up and toggle the onboard LD2 LED.

## 2. Development Environment

| Item | Configuration |
|---|---|
| Board | ST NUCLEO-F446RE |
| MCU | STM32F446RE (ARM Cortex-M4) |
| IDE | STM32CubeIDE |
| Framework | STM32 HAL |
| RTOS | FreeRTOS / CMSIS-RTOS2 |
| HAL Timebase | TIM6 |
| RTOS Tick | SysTick |
| Button | B1 / PC13 |
| LED | LD2 / PA5 |
| EXTI IRQ | EXTI15_10_IRQn |
| EXTI Preemption Priority | 5 |

## 3. System Architecture

```text
B1 Button (PC13)
       |
       v
EXTI15_10_IRQHandler()
       |
       v
HAL_GPIO_EXTI_Callback()
       |
       | osSemaphoreRelease()
       v
Binary Semaphore
       |
       | osSemaphoreAcquire()
       v
FreeRTOS Default Task
       |
       v
HAL_GPIO_TogglePin()
       |
       v
LD2 LED (PA5)
```

The interrupt callback only signals the event. LED control is performed by the FreeRTOS task.

## 4. Implementation

### 4.1 GPIO and EXTI

- PC13 configured as GPIO external interrupt, falling-edge triggered.
- PA5 configured as push-pull GPIO output.
- EXTI15_10 interrupt enabled with preemption priority 5.

### 4.2 Binary Semaphore

The semaphore is initialized after `osKernelInitialize()` and before `osKernelStart()`.

```c
buttonSemHandle = osSemaphoreNew(1, 0, NULL);

if (buttonSemHandle == NULL)
{
    Error_Handler();
}
```

The maximum semaphore count is 1, and its initial count is 0.

### 4.3 Interrupt Callback

```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    static uint32_t last_tick = 0;

    if (GPIO_Pin != B1_Pin || buttonSemHandle == NULL)
    {
        return;
    }

    uint32_t now = HAL_GetTick();

    if ((uint32_t)(now - last_tick) < 80U)
    {
        return;
    }

    last_tick = now;

    (void)osSemaphoreRelease(buttonSemHandle);
}
```

A basic 80 ms software debounce filter is used to reduce repeated button events.

### 4.4 FreeRTOS Task

```c
for (;;)
{
    if (osSemaphoreAcquire(buttonSemHandle, osWaitForever) == osOK)
    {
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    }
}
```

The task remains blocked while the semaphore is unavailable. It becomes ready when the semaphore is released.

## 5. Hardware Verification

The firmware was programmed and verified successfully using ST-LINK.

| Test | Expected Behavior | Result |
|---|---|---|
| Firmware programming | Flash and verify successful | PASS |
| Initial state | LD2 remains in initialized state | PASS |
| First short press | LD2 toggles once | PASS |
| Second short press | LD2 toggles again | PASS |
| Long press (~2 seconds) | No continuous LED toggling | PASS |

**Hardware functional verification: PASS**

## 6. Key Findings

1. GPIO EXTI can notify a FreeRTOS task through a binary semaphore.
2. `osSemaphoreRelease()` provides event signaling from the interrupt callback.
3. `osSemaphoreAcquire(..., osWaitForever)` blocks the task without continuously polling the button.
4. The scheduler can run other ready tasks while the LED task is blocked.
5. Separating ISR signaling from task execution keeps interrupt processing short.
6. TIM6 provides the STM32 HAL timebase while SysTick is used for the FreeRTOS kernel tick.

## 7. Limitations

The following were not verified in this lab:

- High-frequency interrupt stress testing
- Semaphore overflow or event-loss behavior
- Multiple tasks competing for the same semaphore
- Detailed scheduler timing measurements
- Comprehensive mechanical switch debounce testing

The experiment verifies basic EXTI-to-task synchronization, not real-time performance under heavy load.

## 8. Status

**PASS — 2026-11-08**

Scheduled lesson: 2026-11-04 (Wednesday).

Hardware tests were completed successfully on NUCLEO-F446RE.