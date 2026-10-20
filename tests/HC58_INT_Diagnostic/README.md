# Big Dirty MC68HC58 INT Diagnostic

This is one complete standalone diagnostic sketch for the first HC58 interrupt test discussed for Big Dirty. “First” identifies this as the first test in the interrupt investigation; it is **not** a partial build or the first installment of several required sketches.

The TFT, touch controller, SD card, sensors, warning inputs, dimmer, page manager, and normal Big Dirty application are not used.

## What the one sketch does

After USB Serial opens, it automatically runs all three comparison modes with 20 Mode 01 PID 0B (MAP) requests per mode:

1. **MASKED_POLL** — current `0x12` configuration, interrupts masked, timed RxFIFO polling.
2. **IRQ_POLL** — `0x02` configuration, default interrupts enabled, same timed polling.
3. **IRQ_ONLY** — `0x02` configuration, and RxFIFO is not serviced until the Teensy ISR records a falling edge on INT.

That gives a control, proves correlation, and then proves whether INT can actually drive reception—all in the same build.

## Exact configuration change

```text
0x12 = 0001 0010   IMSK=1, interrupts disabled
0x02 = 0000 0010   IMSK=0, default interrupts enabled
```

Every other configuration field stays unchanged. IMOD remains 0, so only the HC58’s default interrupt sources are enabled.

The diagnostic preserves the current Big Dirty initialization command (`0x1A`) and legacy post-configuration cycle in all three modes. Therefore IMSK is the only A/B configuration difference.

## Hardware and pins

Target: Teensy 4.1 with the Fixed Until Broken Rev C J1850 module through the existing Big Dirty interface board.

| Signal | Teensy pin |
|---|---:|
| HC58 INT | 0 |
| HC58 MISO | 1 |
| HC58 RST | 24 |
| HC58 CS | 25 |
| HC58 MOSI | 26 |
| HC58 SCK | 27 |

The existing interface board level-shifts the Rev C module’s 5 V logic to the Teensy side. No module resistor modification is part of this test.

## Running it

1. Open `BigDirty_HC58_INT_Diagnostic/BigDirty_HC58_INT_Diagnostic.ino` in Arduino IDE.
2. Select Teensy 4.1 and USB type Serial.
3. Upload it.
4. Open USB Serial Monitor at 115200 baud.
5. The sketch waits for the monitor, counts down three seconds, and runs the full test automatically.
6. Keep the vehicle/J1850 bus in the same active condition for all three batches.

The full automatic run takes about one minute.

## Evidence produced

Each request logs:

- mode and request number
- whether the expected PCM MAP reply was found
- decoded MAP kPa
- every ISR-recorded INT timestamp relative to request transmission
- INT level before and after the request
- raw frame bytes
- HC58 completion code and decoded ERRF/RFO/TMS/IFR/IFRC/ERRC fields
- FIFO item and SPI service counts
- timeout and cleanup status

Each mode ends with a summary. The complete run ends with explicit PASS/FAIL verdicts.

## Strict IRQ-only rule

In `IRQ_ONLY`, the sketch does not poll the RxFIFO while waiting. It services the HC58 only after the falling-edge ISR increments the request’s interrupt count.

If no interrupt arrives before the timeout:

- the request is marked failed first
- FIFO cleanup may then be performed only to prepare for the next request
- cleanup can never turn a timeout into a successful result

This makes `IRQ_ONLY` evidence materially different from “INT happened while polling still found the reply.”

## Expected proof pattern

A clean result should show:

- `MASKED_POLL`: valid MAP replies and zero IRQ edges
- `IRQ_POLL`: valid MAP replies and one or more correlated IRQ edges for every request
- `IRQ_ONLY`: valid MAP replies for every request, no timeouts, and no fallback cleanup
- `OVERALL_INT_PROOF=PASS`

The HC58 receives its own transmitted request as well as the PCM response, so more than one interrupt/frame per PID request is plausible. The log records the actual behavior instead of assuming a fixed interrupt count.

## Commands after the automatic test

```text
R  rerun complete 20/20/20 proof suite
M  run 100 masked + polling requests
P  run 100 interrupt-enabled + polling requests
I  run 100 interrupt-enabled IRQ-only requests
H  print help
```
