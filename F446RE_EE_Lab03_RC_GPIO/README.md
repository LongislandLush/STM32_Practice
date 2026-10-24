# STM32F446RE — GPIO RC Timing Lab

**EE Foundations · Lab 03 | 2026-10-24**

## Objective
Generate a ~10 Hz square wave on PA8 and measure RC output edge delays with a logic analyzer.

## Setup
- **Board:** NUCLEO-F446RE (STM32 HAL / STM32CubeIDE)
- **GPIO:** PA8, push-pull; HIGH 50 ms / LOW 50 ms
- **Measurement:** Logic 2, 1 MS/s; D0 = PA8, D1 = RC output, common GND
- **Circuit:** `PA8 → R → Vout`; capacitor from `Vout` to GND

```c
while (1) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_Delay(50);
}
```

## Results

| Configuration | Nominal τ = RC | Rising delay | Falling delay |
|---|---:|---:|---:|
| 10 kΩ + 100 nF | 1 ms | 427 µs | 828 µs |
| 10 kΩ + two 100 nF capacitors in series (≈50 nF) | 0.5 ms | 221 µs | 422 µs |
| Two 10 kΩ resistors in series (20 kΩ) + 100 nF | 2 ms | 628 µs | 1830 µs |

The series-capacitor test approximately halved both delays. Doubling resistance increased both delays, but the rising-edge delay did not double.

## Debugging Notes
Initially, the analyzer showed no GPIO transitions. Debugging confirmed `uwTick` advanced and GPIOA ODR bit 8 switched between 0 and 1. Static analyzer tests correctly detected GND as LOW and 3.3 V as HIGH. Replacing a faulty yellow jumper-wire connection restored the waveform.

## Takeaways
- Check wiring and measurement equipment before changing working firmware.
- Logic analyzers measure **digital threshold-crossing delays**, not the analog RC waveform or τ directly.
- Actual analyzer thresholds and component values were not characterized; the rising/falling asymmetry remains open for investigation with an oscilloscope.
