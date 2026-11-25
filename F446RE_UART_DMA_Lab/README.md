# STM32 UART TX DMA Lab — NUCLEO-F446RE

- **Project:** STM32 Renaissance — Lesson 04
- **Board:** NUCLEO-F446RE
- **MCU:** STM32F446RET6
- **Development Environment:** STM32CubeIDE
- **Framework:** STM32 HAL
- **Date:** 2026-11-25
- **Status:** PASS — Hardware Verification

## 1. Objective

This lab demonstrates non-blocking UART transmission using Direct Memory Access (DMA) on the STM32F446RE microcontroller.

The primary objectives are to:

- Configure USART2 TX DMA using STM32CubeMX.
- Transmit UART data using `HAL_UART_Transmit_DMA()`.
- Detect transmission completion through an interrupt-driven callback.
- Verify UART busy-state handling during an active DMA transfer.
- Track successful transmissions, completion events, busy responses, and unexpected errors.
- Understand DMA buffer lifetime and asynchronous peripheral operation.

This experiment does not use FreeRTOS. DMA functionality is verified independently before introducing RTOS synchronization.

## 2. Hardware and Software Environment

| Component | Configuration |
|---|---|
| Development board | NUCLEO-F446RE |
| Microcontroller | STM32F446RET6 |
| IDE | STM32CubeIDE |
| Firmware framework | STM32 HAL |
| UART peripheral | USART2 |
| UART TX pin | PA2 |
| UART RX pin | PA3 |
| Baud rate | 115200 |
| UART format | 8N1 |
| Serial interface | ST-LINK Virtual COM Port |
| COM port | COM3 |
| Serial terminal | MobaXterm |
| Debug interface | ST-LINK |
| RTOS | Not used |

## 3. DMA Configuration

USART2 TX is configured for memory-to-peripheral DMA transmission.

| Parameter | Configuration |
|---|---|
| DMA request | USART2_TX |
| DMA controller | DMA1 |
| DMA stream | Stream 6 |
| DMA channel | Channel 4 |
| Direction | Memory to Peripheral |
| Mode | Normal |
| Peripheral increment | Disabled |
| Memory increment | Enabled |
| Peripheral data width | Byte |
| Memory data width | Byte |
| Priority | Low |

### 3.1 Interrupt Configuration

The intended interrupt configuration includes:

- DMA1 Stream 6 global interrupt
- USART2 global interrupt

In STM32 HAL normal-mode UART DMA transmission, the DMA transfer-complete interrupt indicates that the DMA controller has finished transferring data to the UART peripheral.

The UART transmission-complete interrupt is subsequently used to detect completion of the final UART frame.

Both interrupt paths are important for the HAL transmission-completion mechanism.

### 3.2 Initialization Sequence

The expected peripheral initialization sequence is:

```c
MX_GPIO_Init();
MX_DMA_Init();
MX_USART2_UART_Init();
```

DMA initialization must be performed before UART initialization when required by the generated HAL configuration.

## 4. System Architecture

```text
Application / Main Loop
          |
          v
HAL_UART_Transmit_DMA()
          |
          v
    DMA Controller
          |
          | Memory-to-peripheral transfer
          v
     USART2 TX
          |
          v
ST-LINK Virtual COM Port
          |
          v
    Serial Terminal
```

The transmission-completion notification follows a separate interrupt-driven path:

```text
DMA Transfer Complete
          |
          v
     DMA Interrupt
          |
          v
UART Transmission Complete
          |
          v
     UART Interrupt
          |
          v
HAL_UART_TxCpltCallback()
          |
          v
    txDoneCount++
```

These diagrams represent the intended HAL operation rather than measured hardware timing.

## 5. Firmware Implementation

### 5.1 Transmission Buffers

```c
static uint8_t txBuffer[] =
    "UART DMA transmission test "
    "abcdefghijklmnopqrstuvwxyz "
    "0123456789\r\n";

static uint8_t busyBuffer[] =
    "UNEXPECTED SECOND TX\r\n";
```

Static storage duration ensures that the transmission buffers remain valid throughout the asynchronous DMA operation.

The active transmission buffer must not be modified or reused before transmission completion.

### 5.2 Diagnostic Counters

```c
volatile uint32_t txStartCount = 0;
volatile uint32_t txDoneCount = 0;
volatile uint32_t txBusyCount = 0;
volatile uint32_t txErrorCount = 0;
```

| Counter | Purpose |
|---|---|
| `txStartCount` | Counts successful DMA transmission requests |
| `txDoneCount` | Counts transmission-completion callbacks |
| `txBusyCount` | Counts expected busy responses during the boundary test |
| `txErrorCount` | Counts unexpected API return states |

The counters are observable through STM32CubeIDE Live Expressions.

### 5.3 Transmission Completion Callback

```c
void HAL_UART_TxCpltCallback(
    UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        txDoneCount++;
    }
}
```

The callback performs minimal processing and does not execute blocking UART operations or delays.

### 5.4 Main Loop

```c
HAL_StatusTypeDef status;

status = HAL_UART_Transmit_DMA(
    &huart2,
    txBuffer,
    sizeof(txBuffer) - 1U
);

if (status == HAL_OK)
{
    txStartCount++;

    /* Boundary test: first transfer only */
    if (txStartCount == 1U)
    {
        status = HAL_UART_Transmit_DMA(
            &huart2,
            busyBuffer,
            sizeof(busyBuffer) - 1U
        );

        if (status == HAL_BUSY)
        {
            txBusyCount++;
        }
        else
        {
            txErrorCount++;
        }
    }
}
else
{
    txErrorCount++;
}

HAL_Delay(1000);
```

The one-second delay controls the test transmission interval.

It is not required by DMA itself and is not used as a formal transmission-completion synchronization mechanism.

## 6. Normal Transmission Test

### 6.1 Test Procedure

1. Build the firmware.
2. Download the firmware to the NUCLEO-F446RE.
3. Start the application.
4. Open COM3 using MobaXterm.
5. Observe the UART output.
6. Monitor the diagnostic counters through STM32CubeIDE Live Expressions.

### 6.2 Observed UART Output

```text
UART DMA transmission test abcdefghijklmnopqrstuvwxyz 0123456789
UART DMA transmission test abcdefghijklmnopqrstuvwxyz 0123456789
UART DMA transmission test abcdefghijklmnopqrstuvwxyz 0123456789
UART DMA transmission test abcdefghijklmnopqrstuvwxyz 0123456789
UART DMA transmission test abcdefghijklmnopqrstuvwxyz 0123456789
UART DMA transmission test abcdefghijklmnopqrstuvwxyz 0123456789
UART DMA transmission test abcdefghijklmnopqrstuvwxyz 0123456789
UART DMA transmission test abcdefghijklmnopqrstuvwxyz 0123456789
```

Eight complete transmission messages were visible in the captured terminal output.

No corrupted or truncated messages were observed in the captured output.

**Result: PASS**

## 7. Boundary Test — UART Busy Handling

### 7.1 Test Objective

Verify that an additional UART DMA transmission request is rejected while an existing transmission is still active.

### 7.2 Test Method

Immediately after the first successful DMA transmission request, a second request is issued using `busyBuffer`.

The expected HAL return status is:

```c
HAL_BUSY
```

The expected behavior is:

- The original DMA transmission continues.
- The second transmission request is rejected.
- `txBusyCount` increments once.
- `txErrorCount` remains zero.
- Subsequent periodic transmissions continue normally.

### 7.3 Observed Results

The debugger showed:

```text
txBusyCount  = 0x1
txErrorCount = 0x0
```

The following unexpected message was not observed in the captured terminal output:

```text
UNEXPECTED SECOND TX
```

Normal UART transmission continued after the busy-state test.

**Result: PASS**

## 8. Hardware Verification Results

**Verification date:** 2026-11-25

The following values were captured using STM32CubeIDE Live Expressions:

| Variable | Hexadecimal | Decimal |
|---|---|---|
| `txStartCount` | 0x8 | 8 |
| `txDoneCount` | 0x8 | 8 |
| `txBusyCount` | 0x1 | 1 |
| `txErrorCount` | 0x0 | 0 |

The observed transmission-completion ratio was:

```text
Completed transmissions / Successful requests
= 8 / 8
= 100%
```

This result applies only to the captured verification interval and does not represent a long-duration reliability measurement.

### 8.1 Verification Summary

| Verification item | Result |
|---|---|
| Firmware build and download | PASS |
| UART DMA transmission | PASS |
| Complete UART message output | PASS |
| Transmission-completion callback | PASS |
| Start count equals completion count | PASS |
| Expected busy response | PASS |
| Rejection of the second transmission request | PASS |
| Continued operation after busy response | PASS |
| Unexpected API error count equals zero | PASS |

**Final Result: PASS — Hardware Verification**

## 9. Key Technical Findings

### 9.1 Non-blocking Transmission

Unlike polling-based UART transmission, DMA-based transmission allows the application to initiate a transfer without continuously waiting for each byte to be transmitted.

DMA reduces CPU involvement in data movement but does not increase the configured UART baud rate.

### 9.2 Buffer Lifetime

DMA accesses the transmission buffer asynchronously.

The buffer must remain valid and its contents must remain stable until the transfer is complete.

Static storage duration is used in this experiment to avoid premature buffer lifetime termination.

### 9.3 Peripheral State Management

UART DMA transmission introduces a peripheral state transition:

```text
READY
  |
  | HAL_UART_Transmit_DMA() -> HAL_OK
  v
BUSY_TX
  |
  | Additional request -> HAL_BUSY
  |
  | Transmission complete
  v
READY
```

The busy response is an expected resource-management condition rather than necessarily a hardware failure.

### 9.4 Interrupt-driven Completion

The transmission-completion callback provides an asynchronous notification that can later be used to:

- Release a transmission buffer.
- Notify a waiting RTOS task.
- Start another queued transmission.
- Update diagnostic statistics.

### 9.5 Diagnostic Counter Limitations

The diagnostic counters provide visibility into API return states and completion callbacks.

They do not independently verify every possible UART or DMA hardware error.

The `volatile` qualifier also does not provide general-purpose RTOS synchronization or atomicity guarantees.

## 10. Relationship to Previous Labs

| Previous lab | Related concept |
|---|---|
| UART Polling Echo | Basic UART communication |
| UART Interrupt Echo | Interrupt-driven peripheral operation |
| EXTI + FreeRTOS Semaphore | Event notification |
| EXTI + FreeRTOS Queue | Structured event delivery |
| Queue Full + Drop Counter | Resource exhaustion handling |
| UART TX DMA | Asynchronous I/O and peripheral state management |

The previous Queue Full experiment verified resource exhaustion using `osErrorResource`.

This experiment extends the same defensive programming principle to UART DMA using `HAL_BUSY`.

## 11. Limitations

The following were not evaluated:

- Long-duration UART DMA reliability
- Maximum sustained throughput
- CPU utilization comparison between polling and DMA
- Logic analyzer timing measurements
- UART or DMA hardware error injection
- UART RX DMA
- Circular DMA operation
- UART IDLE line detection
- FreeRTOS synchronization
- Multiple concurrent UART producers

The CubeMX configuration and implementation examples in this document describe the planned lab setup. The hardware verification results are based on the observed serial output and debugger counters.

## 12. Future Learning Direction

Potential follow-up experiments include:

1. UART RX DMA in normal mode.
2. UART RX DMA with IDLE line detection.
3. Variable-length UART frame reception.
4. Circular DMA and ring-buffer management.
5. UART DMA completion notification using FreeRTOS.
6. Queue-based asynchronous UART transmission.

These experiments are not part of the current verification scope.

## 13. Relevance to Embedded Linux and Device Drivers

The concepts verified in this experiment are applicable to broader embedded systems and device-driver development:

- Interrupt-driven I/O
- Asynchronous peripheral operation
- DMA buffer ownership and lifetime
- Resource busy-state handling
- Completion notification
- Driver state management
- Diagnostic instrumentation

Although STM32 HAL and Linux kernel DMA APIs use different interfaces, the underlying principles of asynchronous data transfer and resource management are closely related.

---

**Final Status: PASS — 2026-11-25**

UART TX DMA transmission, interrupt-driven completion, and busy-state handling were successfully demonstrated on the NUCLEO-F446RE development board.