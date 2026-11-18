# STM32F446RE — EXTI to FreeRTOS Message Queue Lab

**Project:** STM32 Practice / STM32 Renaissance  
**Board:** ST NUCLEO-F446RE  
**MCU:** STM32F446RE (ARM Cortex-M4)  
**Framework:** STM32 HAL + FreeRTOS / CMSIS-RTOS2  
**Date:** 2026-11-11  
**Status:** Core Functionality PASS; Queue Full Verification PASS; Long-Press Test Pending

## 1. Objective

Implement event-driven communication between a GPIO external interrupt (EXTI) and a FreeRTOS task using a CMSIS-RTOS2 message queue.

Unlike the previous binary semaphore experiment, this lab transfers structured event data containing:

- A sequential button event ID
- The timestamp of the button event in milliseconds

The receiving task toggles the onboard LED and prints the event information through UART.

## 2. Development Environment

| Item | Configuration |
|---|---|
| Development board | NUCLEO-F446RE |
| MCU | STM32F446RE |
| IDE | STM32CubeIDE |
| RTOS | FreeRTOS / CMSIS-RTOS2 |
| HAL timebase | TIM6 |
| RTOS tick | SysTick |
| User button | B1 / PC13 |
| LED | LD2 / PA5 |
| Interrupt | EXTI15_10 |
| EXTI priority | 5 |
| UART | USART2 / ST-LINK Virtual COM Port |
| Serial configuration | 115200 baud, 8N1 |
| Message queue capacity | 4 events |
| Message payload size | 8 bytes |

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
       | osMessageQueuePut()
       | { event_id, tick_ms }
       v
FreeRTOS Message Queue
       |
       | osMessageQueueGet()
       v
StartDefaultTask()
       |
       +----> LD2 Toggle (PA5)
       |
       +----> USART2 Event Log
```

The interrupt callback generates event data, while the FreeRTOS task performs the LED and UART operations.

This keeps the interrupt callback relatively short and avoids performing blocking UART transmission inside the ISR.

## 4. Implementation

### 4.1 Button Event Structure

```c
typedef struct
{
    uint32_t event_id;
    uint32_t tick_ms;
} ButtonEvent;
```

Each event occupies 8 bytes on the target platform.

### 4.2 Message Queue Declaration

```c
static osMessageQueueId_t buttonQueueHandle = NULL;
static volatile uint32_t queueDropCount = 0;
```

The queue handle identifies the CMSIS-RTOS2 message queue.

`queueDropCount` records failed attempts to enqueue button events. Declaring it `volatile` does not make updates atomic.

### 4.3 Message Queue Initialization

The queue is created after `osKernelInitialize()` and before `osKernelStart()`.

```c
buttonQueueHandle = osMessageQueueNew(
    4,
    sizeof(ButtonEvent),
    NULL
);

if (buttonQueueHandle == NULL)
{
    Error_Handler();
}
```

The queue can store up to four events. Its payload storage capacity is 32 bytes, excluding RTOS management overhead.

### 4.4 EXTI Callback

```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    static uint32_t last_tick = 0;
    static uint32_t event_count = 0;
    static uint8_t has_last_event = 0;

    if (GPIO_Pin != B1_Pin || buttonQueueHandle == NULL)
    {
        return;
    }

    uint32_t now = HAL_GetTick();

    /* 80 ms software debounce */
    if (has_last_event &&
        (uint32_t)(now - last_tick) < 80U)
    {
        return;
    }

    has_last_event = 1;
    last_tick = now;

    ButtonEvent event = {
        .event_id = event_count + 1U,
        .tick_ms = now
    };

    /* ISR context: timeout must be zero */
    if (osMessageQueuePut(
            buttonQueueHandle,
            &event,
            0U,
            0U) == osOK)
    {
        event_count = event.event_id;
    }
    else
    {
        queueDropCount++;
    }
}
```

The callback uses an 80 ms debounce interval.

`osMessageQueuePut()` copies the event payload into the queue. The callback therefore does not need to preserve the local `event` variable after returning.

The timeout is set to zero because the function is called from interrupt context.

### 4.5 FreeRTOS Task

The default task stack allocation was increased to 1536 bytes for this experiment.

```c
ButtonEvent event;
char message[80];

for (;;)
{
    if (osMessageQueueGet(
            buttonQueueHandle,
            &event,
            NULL,
            osWaitForever) == osOK)
    {
        HAL_GPIO_TogglePin(
            LD2_GPIO_Port,
            LD2_Pin
        );

        int len = snprintf(
            message,
            sizeof(message),
            "Button Event #%lu at %lu ms\r\n",
            (unsigned long)event.event_id,
            (unsigned long)event.tick_ms
        );

        if (len > 0 && len < (int)sizeof(message))
        {
            HAL_UART_Transmit(
                &huart2,
                (uint8_t *)message,
                (uint16_t)len,
                100
            );
        }
    }
}
```

The task blocks on `osMessageQueueGet()` while no messages are available.

When a message arrives, the task receives the event structure, toggles LD2, and sends the formatted event information over UART.

## 5. Hardware Verification

### 5.1 Firmware Execution

The Queue Lab firmware was successfully executed on the NUCLEO-F446RE.

The debug session was configured to use the Queue Lab ELF rather than the previously copied Semaphore Lab configuration.

**Result: PASS**

### 5.2 UART Event Output

The following output was observed on the ST-LINK Virtual COM Port (COM3):

```text
Button Event #1 at 3515 ms
Button Event #2 at 4215 ms
Button Event #3 at 4570 ms
```

**Result: PASS**

The observed event IDs were sequential, and the timestamps increased monotonically.

### 5.3 LED Toggle

The onboard LD2 LED toggled successfully in response to the button events.

**Result: PASS**

### 5.4 Verification Summary

| Test | Expected Result | Actual Status |
|---|---|---|
| Queue Lab firmware execution | Firmware runs on target | PASS |
| EXTI event handling | Button events trigger processing | PASS |
| Queue event transfer | Event data reaches receiving task | PASS |
| UART event ID | Sequential IDs | PASS |
| UART timestamp | Increasing timestamps | PASS |
| LD2 toggle | LED changes state on button events | PASS |
| Long press (~2 seconds) | No continuous event generation | Not yet reported |
| Queue drop counter | Inspect failed enqueue attempts | Not yet reported |
| Queue-full stress test | Detect and handle queue capacity limit | Not tested |

**Core functional verification: PASS**

## 6. Key Findings

### 6.1 Binary Semaphore vs. Message Queue

The previous binary semaphore lab provided synchronization without carrying an event payload.

This message queue lab transfers a structured `ButtonEvent` containing an event ID and timestamp.

### 6.2 ISR-to-Task Communication

The interrupt callback creates and sends the event.

The FreeRTOS task receives and processes it.

Blocking UART transmission is performed in task context instead of interrupt context.

### 6.3 Message Copy Semantics

`osMessageQueuePut()` copies the message contents into queue-managed storage.

The receiving task retrieves the data into its own `ButtonEvent` variable.

### 6.4 Task Blocking

Using `osWaitForever` allows the task to remain blocked while the queue is empty.

The scheduler can execute other ready tasks rather than continuously polling the button.

### 6.5 Queue Capacity and Event Loss

The queue capacity is four messages.

If the queue cannot accept a new event, the application increments `queueDropCount`.

The actual queue-full behavior and event-loss characteristics have not yet been experimentally verified.

## 7. Limitations

The following remain outside the verified scope:

- Queue-full stress testing
- High-frequency interrupt event handling
- Event-loss and throughput measurements
- Detailed task scheduling latency measurements
- Task stack high-water-mark measurements
- Comprehensive button debounce testing
- DMA-based UART transmission
- Long-press boundary test (pending confirmation)

The lab demonstrates basic event-driven ISR-to-task communication using a message queue.

It does not establish performance guarantees under heavy interrupt load.

## 8. Conclusion

The NUCLEO-F446RE successfully demonstrated event-driven communication from a GPIO external interrupt to a FreeRTOS task using a CMSIS-RTOS2 message queue.

The application transferred sequential button event IDs and timestamps, toggled the onboard LED, and printed event information through USART2.

**Final status: Core Functionality PASS — 2026-11-11.**

## 9. Queue Full and Drop Counter Verification

**Date:** 2026-11-18  
**Board:** NUCLEO-F446RE  
**RTOS:** FreeRTOS / CMSIS-RTOS2  
**Result:** PASS

### 9.1 Objective

Verify message queue capacity handling and event-drop detection under a controlled burst of interrupt-generated messages.

The experiment uses a queue with a capacity of four messages and deliberately attempts to enqueue five messages during a single EXTI callback.

The objective is to confirm that:

- The first four messages are accepted.
- The fifth message is rejected when the queue is full.
- The drop counter increments when `osMessageQueuePut()` returns `osErrorResource`.
- Normal single-event behavior is preserved after disabling burst mode.

### 9.2 Test Configuration

| Parameter | Stress Mode | Normal Mode |
|---|---|---|
| Queue capacity | 4 | 4 |
| `QUEUE_TEST_BURST` | 5 | 1 |
| Event source | B1 / PC13 EXTI | B1 / PC13 EXTI |
| Event consumer | FreeRTOS default task | FreeRTOS default task |
| Output | USART2 / COM3 | USART2 / COM3 |
| UART configuration | 115200, 8N1 | 115200, 8N1 |

The experiment retains the existing 80 ms software debounce mechanism.

### 9.3 Implementation

A configurable burst count was added:

```c
#define QUEUE_TEST_BURST 5U
```

Inside `HAL_GPIO_EXTI_Callback()`, a loop attempts to enqueue multiple button events:

```c
for (uint32_t i = 0; i < QUEUE_TEST_BURST; i++)
{
    ButtonEvent event = {
        .event_id = event_count + 1U,
        .tick_ms = now
    };

    osStatus_t status = osMessageQueuePut(
        buttonQueueHandle,
        &event,
        0U,
        0U
    );

    if (status == osOK)
    {
        event_count = event.event_id;
    }
    else if (status == osErrorResource)
    {
        queueDropCount++;
    }
    else
    {
        /* Unexpected API error */
    }
}
```

The ISR uses a zero timeout and does not perform UART transmission.

The receiving task reports the accumulated drop count:

```c
int len = snprintf(
    message,
    sizeof(message),
    "Event #%lu at %lu ms | FullDrop=%lu\r\n",
    (unsigned long)event.event_id,
    (unsigned long)event.tick_ms,
    (unsigned long)queueDropCount
);
```

### 9.4 Stress Mode — Hardware Results

**Configuration:** `QUEUE_TEST_BURST = 5U`

Three button-triggered bursts were observed.

**Actual UART output:**

```text
Event #1  at 7266 ms  | FullDrop=1
Event #2  at 7266 ms  | FullDrop=1
Event #3  at 7266 ms  | FullDrop=1
Event #4  at 7266 ms  | FullDrop=1
Event #5  at 10202 ms | FullDrop=2
Event #6  at 10202 ms | FullDrop=2
Event #7  at 10202 ms | FullDrop=2
Event #8  at 10202 ms | FullDrop=2
Event #9  at 11550 ms | FullDrop=3
Event #10 at 11550 ms | FullDrop=3
Event #11 at 11550 ms | FullDrop=3
Event #12 at 11550 ms | FullDrop=3
```

**Observed results:**

| Burst | Accepted events | Accumulated FullDrop |
|---|---|---|
| 1 | #1–#4 | 1 |
| 2 | #5–#8 | 2 |
| 3 | #9–#12 | 3 |

A total of 12 messages were received across three bursts, while the accumulated drop counter increased to 3.

Each burst produced four accepted events and one rejected enqueue attempt.

All four accepted messages within each burst shared the same timestamp, consistent with capturing `HAL_GetTick()` once per callback.

**Stress Mode Result: PASS**

### 9.5 Normal Mode — Hardware Results

The burst configuration was changed to:

```c
#define QUEUE_TEST_BURST 1U
```

After rebuilding and restarting the firmware, the following UART output was observed:

```text
Event #1 at 4898 ms | FullDrop=0
Event #2 at 5927 ms | FullDrop=0
Event #3 at 6307 ms | FullDrop=0
Event #4 at 6659 ms | FullDrop=0
Event #5 at 7143 ms | FullDrop=0
Event #6 at 7442 ms | FullDrop=0
```

Six sequential events were received, and `FullDrop` remained zero.

**Normal Mode Result: PASS**

### 9.6 Key Findings

1. A CMSIS-RTOS2 message queue with a capacity of four can hold four event messages before becoming full.
2. A fifth enqueue attempt from the same ISR can return `osErrorResource` when no queue space is available.
3. Tracking failed enqueue attempts provides visibility into message loss under burst conditions.
4. Sequential event IDs alone do not prove that no messages were dropped, because the current implementation increments `event_count` only after a successful enqueue.
5. The receiving task can process queued events after the ISR returns.
6. Restoring `QUEUE_TEST_BURST` to 1 preserves the original single-event behavior.

### 9.7 Limitations

This experiment verified controlled queue saturation, not sustained high-frequency interrupt performance.

The following were not evaluated:

- Queue throughput under continuous load
- Task scheduling latency
- Concurrent producers
- ISR execution-time measurements
- Stack high-water-mark measurements
- Long-duration event-loss statistics

### 9.8 Final Verification Summary

| Verification Item | Result |
|---|---|
| Controlled burst of five enqueue attempts | PASS |
| Four accepted messages per burst | PASS |
| Queue-full drop counter increment | PASS |
| Three consecutive burst observations | PASS |
| Return to single-event mode | PASS |
| Zero drops in normal mode | PASS |

**Final Result: PASS — 2026-11-18**

The Queue Full and Drop Counter experiment successfully demonstrated controlled queue saturation, error handling, and restoration of normal operation on the NUCLEO-F446RE.

## 10. Future Learning Direction

Potential follow-up topics include:

- Queue-full and event-loss behavior
- UART DMA transmission
- UART DMA with IDLE line detection
- FreeRTOS task synchronization and timing analysis
- ISR-to-task communication patterns in embedded Linux drivers

These are future learning directions, not completed experiments.