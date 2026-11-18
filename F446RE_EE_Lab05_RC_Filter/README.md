# EE Foundations — Lab 05-A: STM32 TIM1 PWM × RC Low-pass Filter

## 1. Overview

This lab investigates the behavior of an RC low-pass filter driven by PWM signals generated using **STM32 TIM1 Channel 1**.

The experiment uses a **NUCLEO-F446RE**, a 10 kΩ resistor, a 100 nF capacitor, and a digital Logic Analyzer.

The main objectives are:

- Generate 10, 100, 500, and 1000 Hz PWM signals with approximately 50% duty cycle.
- Observe the timing difference between the original PWM signal and the RC filter output.
- Compare digital edge delays at different frequencies.
- Perform an A/B test by removing the capacitor.
- Understand the limitations of using a digital Logic Analyzer to investigate analog RC behavior.

**Result: PASS — Digital PWM and RC edge-delay verification completed.**

Analog amplitude attenuation and the -3 dB cutoff frequency were not directly measured.

---

## 2. Hardware and Software

| Item | Description |
|---|---|
| MCU Board | ST NUCLEO-F446RE |
| MCU | STM32F446RE |
| IDE | STM32CubeIDE |
| Timer | TIM1 Channel 1 |
| PWM Output | PA8 |
| Resistor | 10 kΩ |
| Capacitor | 100 nF |
| Logic Analyzer | Digital LA with Saleae Logic 2 |
| LA D0 | GPIO_PA8 |
| LA D1 | RC_VOUT |
| LA GND | STM32 GND |

## 3. Circuit Configuration

```text
                      10 kΩ
STM32 PA8 ───────────/\/\/\─────┬──── LA D1
     │                          │
     └──── LA D0              100 nF
                                │
                               GND

LA GND ─────────────────────── GND
```

The original PWM signal is measured at PA8 (D0), while the filtered signal is measured at RC_VOUT (D1).

### RC Parameters

\[
R=10\,k\Omega
\]

\[
C=100\,nF
\]

Time constant:

\[
\tau=RC=1\,ms
\]

Theoretical cutoff frequency:

\[
f_c=\frac{1}{2\pi RC}\approx159.15\,Hz
\]

The cutoff frequency is defined using the sinusoidal frequency response of the RC filter. It cannot be directly determined from the digital edge-delay measurements in this experiment.

---

## 4. STM32 TIM1 PWM Configuration

PA8 is configured as **TIM1_CH1**, using PWM Mode 1 with active-high polarity and an intended duty cycle of 50%.

The PWM frequency is determined by:

\[
f_{PWM}=\frac{f_{TIM}}{(PSC+1)(ARR+1)}
\]

Assuming an 84 MHz TIM1 clock:

| Parameter | Value |
|---|---:|
| TIM1 Clock | 84 MHz (configuration assumption) |
| Prescaler (PSC) | 839 |
| Counter Clock | 100 kHz |
| Counter Mode | Up |
| PWM Mode | PWM Mode 1 |
| Output Polarity | High |

### Frequency Configuration

| Target Frequency | PSC | ARR | CCR1 |
|---|---:|---:|---:|
| 10 Hz | 839 | 9999 | 5000 |
| 100 Hz | 839 | 999 | 500 |
| 500 Hz | 839 | 199 | 100 |
| 1000 Hz | 839 | 99 | 50 |

### PWM Frequency Control

```c
static void PWM_SetFrequency(uint32_t frequency_hz)
{
    const uint32_t counter_clock_hz = 100000U;

    if (frequency_hz == 0U)
    {
        return;
    }

    uint32_t period = counter_clock_hz / frequency_hz;

    if ((period < 2U) || (period > 65536U))
    {
        return;
    }

    __HAL_TIM_SET_AUTORELOAD(&htim1, period - 1U);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, period / 2U);

    HAL_TIM_GenerateEvent(&htim1, TIM_EVENTSOURCE_UPDATE);
}
```

The PWM output is started using:

```c
HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
```

For each measurement, the PWM frequency was configured to the target test frequency.

The actual clock configuration should be verified against the STM32CubeIDE `.ioc` file before reusing the timer parameters in another project.

---

## 5. Build Verification

During initial development, the build failed because an incorrect HAL function name was used:

```c
__HAL_TIM_GENERATE_EVENT(&htim1, TIM_EVENTSOURCE_UPDATE);
```

The issue was resolved by replacing it with:

```c
HAL_TIM_GenerateEvent(&htim1, TIM_EVENTSOURCE_UPDATE);
```

Final build result:

```text
Project: F446RE_EE_Lab05_RC_Filter
Configuration: Debug

Build Finished.
0 errors, 0 warnings.
```

**Status: PASS**

---

## 6. Experimental Results

### 6.1 PWM Frequency Verification

| Target Frequency | Measured Frequency | Duty Cycle | Period |
|---|---:|---:|---:|
| 10 Hz | 10.03 Hz | ~50% | 99.695 ms |
| 100 Hz | 100.402 Hz | 50.0% | 9.960 ms |
| 500 Hz | 501.756 Hz | 49.97% | 1.993 ms |
| 1000 Hz | ~1005 Hz | ~49.95% | 995 µs |

All four PWM outputs were observed using the digital Logic Analyzer.

**PWM Generation: PASS**

### 6.2 RC Digital Edge Delay

The timing difference between D0 (PA8) and D1 (RC_VOUT) was measured using Logic 2 timing markers.

| PWM Frequency | Rising Edge Delay | Falling Edge Delay | Observation |
|---|---:|---:|---|
| 10 Hz | 255 µs | 907 µs | Both edges delayed |
| 100 Hz | 245 µs | 889 µs | Similar delays to 10 Hz |
| 500 Hz | Nearly coincident (visual) | ~559 µs | Asymmetric digital transitions |
| 1000 Hz | Nearly coincident (visual) | Nearly coincident (visual) | D1 still toggled |

The rising and falling edge delays were not equal.

This behavior may be influenced by the Logic Analyzer's input thresholds, the RC waveform's initial voltage, component tolerances, and measurement conditions.

The measured digital edge delays must **not** be interpreted directly as the analog RC time constant.

### 6.3 Logic Analyzer Channel Verification

To check the Logic Analyzer channels, D0 and D1 were temporarily connected to the same PA8 output node.

At approximately 500 Hz:

- Both channels displayed the same PWM waveform.
- Rising edges appeared aligned.
- Falling edges appeared aligned.
- The measured HIGH duration was approximately 997 µs.

**Result: PASS**

No significant channel-to-channel timing offset was visible at the displayed resolution.

---

## 7. A/B Test — Capacitor Removal

An additional controlled experiment was performed to investigate whether the capacitor contributed to the observed edge delay.

### Test A: Capacitor Connected

Circuit:

```text
PA8 ── 10 kΩ ── RC_VOUT
                   │
                 100 nF
                   │
                  GND
```

At 10 Hz:

| Measurement | Result |
|---|---:|
| Rising Edge Delay | 255 µs |
| Falling Edge Delay | 907 µs |

### Test B: Capacitor Removed

Circuit:

```text
PA8 ── 10 kΩ ── RC_VOUT
```

The resistor, Logic Analyzer connections, and PWM configuration were retained.

At 10 Hz:

| Measurement | Result |
|---|---|
| PWM Frequency | 10.031 Hz |
| Duty Cycle | 50.01% |
| Period | 99.693 ms |
| HIGH Duration | 49.839 ms |
| Rising Edge Delay | Nearly coincident (visual) |
| Falling Edge Delay | Nearly coincident (visual) |

### A/B Test Conclusion

With the capacitor connected, measurable digital edge delays were observed.

After removing the capacitor, D0 and D1 transitions appeared nearly coincident.

This supports the conclusion that the capacitor load was a major contributor to the observed edge delays.

**A/B Test: PASS (qualitative verification)**

---

## 8. Discussion

### 8.1 Why Does the RC Output Change with Frequency?

The RC filter has a theoretical time constant of 1 ms.

At low PWM frequencies, the capacitor has sufficient time to charge and discharge toward the input's HIGH and LOW levels.

At higher frequencies, the available charging and discharging time becomes shorter relative to the RC time constant.

For an ideal 0–3.3 V, 50% duty-cycle square wave, the approximate steady-state RC output ranges are:

| PWM Frequency | Theoretical Vout Range |
|---|---|
| 10 Hz | Approximately 0–3.3 V |
| 100 Hz | Approximately 0.02–3.28 V |
| 500 Hz | Approximately 0.89–2.41 V |
| 1000 Hz | Approximately 1.25–2.05 V |

These voltage ranges are theoretical predictions, not measured analog voltages.

### 8.2 Why Can D1 Still Toggle at 1000 Hz?

A digital Logic Analyzer converts its input voltage into HIGH or LOW states using input thresholds.

Even when the analog RC output no longer reaches 0 V or 3.3 V, the signal may still cross the analyzer's switching thresholds.

Therefore, the observed digital waveform does not directly reveal the analog output amplitude.

### 8.3 Measurement Limitations

This experiment used a digital Logic Analyzer rather than an oscilloscope.

Consequently:

- Analog charging and discharging curves were not directly captured.
- Actual Vout minimum and maximum voltages were not measured.
- The -3 dB cutoff frequency was not experimentally verified.
- Logic Analyzer switching thresholds were not independently measured.
- High-frequency edge-delay observations are qualitative where precise timing markers were not recorded.

These limitations should be considered when interpreting the results.

---

## 9. Final Verification

| Verification Item | Result |
|---|---|
| STM32CubeIDE Build | PASS |
| TIM1 PWM Generation | PASS |
| 10 / 100 / 500 / 1000 Hz Output | PASS |
| 50% Duty Cycle | PASS |
| RC Digital Edge-Delay Measurement | PASS |
| Logic Analyzer Channel Cross-check | PASS |
| Capacitor Removal A/B Test | PASS |
| Analog Amplitude Measurement | Not performed |
| Experimental -3 dB Cutoff Verification | Not performed |

**Final Status: PASS — Digital timing and RC capacitor-load verification completed.**

---

## 10. Key Learnings

1. STM32 TIM1 hardware PWM can generate stable, configurable PWM signals without relying on software delay loops.
2. PSC, ARR, and CCR determine the timer frequency and PWM duty cycle.
3. An RC low-pass filter introduces frequency-dependent analog behavior.
4. Digital Logic Analyzer edge delays depend on input switching thresholds and must not be treated as direct RC time-constant measurements.
5. A controlled capacitor-removal experiment can help isolate the contribution of capacitive loading.
6. Hardware measurements must be interpreted according to the capabilities and limitations of the measurement instrument.

## 11. Future Improvements

- Measure analog RC output waveforms using an oscilloscope or suitable ADC acquisition setup.
- Characterize the frequency response using sinusoidal inputs.
- Measure the actual -3 dB cutoff frequency.
- Compare different resistor and capacitor values.
- Investigate Logic Analyzer input threshold behavior.

---

**EE Foundations — Hardware for Firmware**

**Lab 05-A: Completed (Digital Verification Scope)**