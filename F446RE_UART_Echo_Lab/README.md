# STM32F446RE — USART2 RX Interrupt Lab

## 1. Objective

Implement interrupt-driven UART reception using STM32 HAL, recognize a simple command, and trigger GPIO actions from the main loop.

This lab extends the previous USART2 Polling Echo experiment.

## 2. Environment

| Item | Configuration |
|---|---|
| Board | NUCLEO-F446RE |
| IDE | STM32CubeIDE |
| Framework | STM32 HAL |
| UART | USART2, 115200 baud, 8N1 |
| Interface | ST-LINK Virtual COM Port |
| Terminal | MobaXterm |
| GPIO | PA5 / LD2 |

## 3. Implementation

### UART RX Interrupt Flow

```text
PC / MobaXterm
      |
      | Send "A1?"
      v
USART2 RX Interrupt
      |
      v
USART2_IRQHandler()
      |
      v
HAL_UART_IRQHandler()
      |
      v
HAL_UART_RxCpltCallback()
      |
      | Parse received byte
      | Increment a1_count
      | Re-arm UART RX
      v
Main Loop
      |
      v
PA5 / LD2 Toggle x3
```

### Key HAL APIs

- `HAL_UART_Receive_IT()` — Start interrupt-driven UART reception.
- `HAL_UART_IRQHandler()` — Handle USART2 interrupt events.
- `HAL_UART_RxCpltCallback()` — Process received bytes and recognize commands.
- `HAL_GPIO_TogglePin()` — Toggle the onboard LED.

The UART callback handles command recognition and event notification. LED delays are handled in the main loop rather than inside the interrupt callback.

## 4. Verification Results

Tested on NUCLEO-F446RE hardware.

| Test | Expected Result | Actual Result |
|---|---|---|
| UART initialization | `UART2 IT READY` displayed | PASS |
| `A1?` command | LD2 toggles three times | PASS |
| LED final state | Opposite of initial state | PASS |
| `A2?` command | No LED action | PASS |
| `A2?` followed by `A1?` | Valid command still works | PASS |

**Core functional verification: PASS**

## 5. Key Findings

1. UART reception can be handled using interrupts instead of blocking polling.
2. `HAL_UART_Receive_IT()` must be re-armed after each completed reception in this implementation.
3. Interrupt callbacks should remain short.
4. `HAL_Delay()` inside an ISR may block execution or stall if the HAL tick cannot advance.
5. Shared event counters allow the main loop to handle events outside interrupt context.
6. Three GPIO toggles result in an inverted final LED state.

## 6. Limitations

The following have not been verified:

- UART overrun and error recovery
- High-throughput continuous reception
- UART DMA and Receive-to-Idle
- FreeRTOS task synchronization
- Comprehensive command parser edge cases

The current implementation is intended for basic interrupt-driven UART learning.

## 7. Status

**PASS — 2026-10-28**

Hardware verification completed. GitHub documentation and version control are tracked separately.