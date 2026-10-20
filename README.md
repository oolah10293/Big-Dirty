# Big Dirty

Big Dirty is a standalone Teensy 4.1 instrument-cluster / vehicle-data display built around a 480x320 ST7796S TFT, XPT2046 touch controller, and Motorola MC68HC58 J1850 VPW interface.

The integrated firmware talks directly to a 2005-era GM 8.1L PCM over VPW, displays live OBD data, reads several local sensors, handles vehicle warning inputs, and provides a touch-driven DTC read/clear screen.

The repository deliberately keeps the Arduino sketch and all .cpp/.h modules in the sketch folder. That matches the known working development layout and avoids reorganizing a functioning embedded project for cosmetic reasons.

## Authoritative firmware

The recovered final baseline is **BigDirty_UI_Main - Save 14**, recovered on **2026-09-16** and identified as the final version. Current `main` is Save 14 plus one targeted post-final page-transition fix added on **2026-10-19**.

It contains 31 source files. Compared with the earlier 2026-04-07 GitHub snapshot:

- **27 files are byte-for-byte identical**
- **4 files are newer in Save 14**
  - `DTC_Manager.cpp`
  - `Display_ST7796S.cpp`
  - `Page_DTC.cpp`
  - `Page_Env.cpp`

Those four recovered files still match Save 14 exactly at the Git blob level. `PageManager.cpp` now intentionally differs from Save 14: the 200 ms touch/SPI transition guard runs **before** the destination page's `begin()`/full redraw instead of afterward. This addresses an observed ENV -> PID1 failure where live PID1 values appeared over stale ENV labels. Hardware validation of that fix is pending. See [history/FINAL_SAVE_14_MANIFEST.md](history/FINAL_SAVE_14_MANIFEST.md).

Key final-version changes include:

- combined Mode 03 stored-code and Mode 07 pending-code scanning
- duplicate suppression when the same DTC appears in both services
- Mode 04 clear followed by combined Mode 03 + Mode 07 verification
- more defensive DHT20, current-sensor, and transmission-temperature failure handling
- transmission-temperature polling changed to 1.5 seconds
- explicit display-SPI transaction ownership rather than nested transactions
- final DTC UI wording such as **SCANNING CODES** and **NO DIAGNOSTIC CODES**

## Hardware

| Function | Hardware |
|---|---|
| Main MCU | Teensy 4.1 |
| Display | ST7796S, 480x320, SPI |
| Touch | XPT2046 |
| Vehicle data | Motorola MC68HC58 J1850 VPW interface |
| Storage | Teensy 4.1 built-in SD |
| Ambient sensor | DHT20 |
| Alternator current | LA37S200S05-family Hall current transducer interface |
| Transmission temperature | resistive sender + precision reference |
| Backlight control | vehicle dimmer/rheostat voltage input |
| Warning inputs | seat belt, brake, high beam, check engine, 4WD |

The repository also contains the interface-board schematic, connection spreadsheet, current-transducer datasheet, DHT20 datasheet, and a retained sensor-curve reference PDF.

## UI pages

Touching the display advances through the runtime pages.

| Page | Q1 | Q2 | Q3 | Q4 |
|---|---|---|---|---|
| ENV | Ambient temp °F | Humidity % | Alternator A | Trans temp °F |
| PID1 | TPS % | IAT °F | MAP kPa | MAF g/s |
| PID2 | Calculated load % | Timing advance ° | O2 B1 mV | O2 B2 mV |
| PID3 | STFT1 % | LTFT1 % | STFT2 % | LTFT2 % |
| DTC | stored + pending trouble-code list and clear control | | | |

Startup begins on the boot-logo page. After the logo timeout/fade, the cycle is ENV -> PID1 -> PID2 -> PID3 -> DTC -> ENV.

## Important pin summary

### Display / touch

| Signal | Teensy pin |
|---|---:|
| TFT CS | 10 |
| TFT DC | 9 |
| TFT RST | 8 |
| TFT LED/PWM | 23 |
| Touch CS | 22 |
| Touch IRQ | 21 |
| SPI0 MOSI | 11 |
| SPI0 MISO | 12 |
| SPI0 SCK | 13 |

### HC58 / SPI1

| Signal | Teensy pin |
|---|---:|
| HC58 INT | 0 |
| HC58 MISO | 1 |
| HC58 RST | 24 |
| HC58 CS | 25 |
| HC58 MOSI | 26 |
| HC58 SCK | 27 |

The integrated code does not depend on the HC58 interrupt. RX is drained by polling.

### Local analog / I2C inputs

| Function | Teensy pin |
|---|---:|
| Transmission sender | 14 |
| Current-sensor VREF | 15 |
| Current-sensor VOUT | 16 |
| MCP1525 2.5 V reference monitor | 17 |
| DHT20 SDA | 18 |
| DHT20 SCL | 19 |
| Vehicle dimmer sense | 20 |

### Warning inputs

| Warning | Pin | Active state |
|---|---:|---|
| Seat belt | 2 | HIGH |
| Brake | 3 | HIGH |
| High beam | 4 | HIGH |
| Check engine | 6 | LOW |
| 4WD | 7 | LOW |

For the full map, see [docs/PINOUT.md](docs/PINOUT.md).

## Display baseline

The final display driver preserves:

- 480x320 landscape
- SPI at 16 MHz
- MSB first
- SPI Mode 3
- ST7796S MADCTL = 0x28
- RGB565 / 16-bit color
- XPT2046 rotation = 3

Save 14 also makes display SPI ownership explicit: drawing functions open one transaction, keep the touch controller deselected, perform the complete operation, then close the transaction.

## VPW / OBD implementation

The HC58 runs on Teensy SPI1 at 100 kHz, Mode 3.

Mode 01 live-data requests use a functional VPW request beginning with:

    68 6A F1 01 PID

The integrated parser searches for PCM replies beginning with:

    48 6B 10 41 PID

Current live PIDs:

- 04 calculated load
- 06/07/08/09 short/long fuel trims for banks 1 and 2
- 0B MAP
- 0E timing advance
- 0F IAT
- 10 MAF
- 11 TPS
- 14/15 upstream O2 voltage for banks 1 and 2

The original four-PID Arduino Uno timing/logger sketch is preserved as [Four_PIDs_at_once.ino](Four_PIDs_at_once.ino). More detail is in [docs/VPW_HC58.md](docs/VPW_HC58.md).

## DTC support

The final DTC path supports:

- Mode 03 stored-code request
- Mode 07 pending-code request
- merging and deduplicating results from both services
- positive-reply detection
- negative-reply detection
- generic P/C/B/U DTC formatting
- curated labels for common engine/transmission codes
- up to eight displayed unique codes
- Mode 04 clear
- combined Mode 03 + Mode 07 reread after clear

A P0102 MAF-low code was successfully used during project testing.

See [docs/DTC.md](docs/DTC.md).

## Local sensor processing

The final Save 14 ENV page still owns its sensor acquisition.

Intervals:

- DHT20: 1000 ms
- alternator current: 250 ms
- transmission temperature: 1500 ms
- Teensy ADC: 12-bit with 16-sample averaging

Save 14 adds failure handling so invalid/missing sensors fall back to `---` instead of leaving stale values or repeatedly blocking the UI. The DHT20 is abandoned for the current page visit after three failed reads.

A later architectural preference remains to separate acquisition from rendering, but the final working source is preserved as-is rather than rewritten.

See [docs/SENSORS_AND_INPUTS.md](docs/SENSORS_AND_INPUTS.md).

## SD card assets

The five 96x72 warning-icon RAW files have been recovered exactly and are preserved in [sdcard/warning-icons-rgb565.zip](sdcard/warning-icons-rgb565.zip).

Their supplied metadata confirms:

- RGB565
- 16 bits per pixel
- little-endian byte order
- 13,824 bytes per icon

Recovered:

- `brake.raw`
- `checkengine.raw`
- `highbeam.raw`
- `4WD.raw`
- `seatbelt.raw`

Still missing:

- `/bowtie.raw` — 480x320 boot image

See [sdcard/README.md](sdcard/README.md) and [sdcard/WARNING_ASSET_MANIFEST.md](sdcard/WARNING_ASSET_MANIFEST.md).

## Building

This is an Arduino/Teensy sketch layout.

Required target:

- Teensy 4.1

External libraries used by the final source:

- XPT2046_Touchscreen
- a DHT20 library exposing DHT20, begin(), read(), getTemperature(), getHumidity(), and DHT20_OK

The normal Teensy core supplies SPI, SD, Wire, Arduino timing/GPIO, and built-in SD-card support.

See [docs/BUILD_AND_FLASH.md](docs/BUILD_AND_FLASH.md).

## Repository guide

- [DEVELOPMENT.md](DEVELOPMENT.md) — architecture and change discipline
- [CHANGELOG.md](CHANGELOG.md) — recoverable project milestones
- [ROADMAP.md](ROADMAP.md) — preservation and remaining cleanup
- [docs/PROJECT_CONTEXT.md](docs/PROJECT_CONTEXT.md) — goals and design decisions
- [docs/REPOSITORY_MAP.md](docs/REPOSITORY_MAP.md) — source-file roles
- [docs/PINOUT.md](docs/PINOUT.md) — pin assignments
- [docs/UI_AND_DISPLAY.md](docs/UI_AND_DISPLAY.md) — display, touch, pages, warnings
- [docs/VPW_HC58.md](docs/VPW_HC58.md) — VPW interface details
- [docs/DTC.md](docs/DTC.md) — Mode 03/07/04 behavior
- [docs/SENSORS_AND_INPUTS.md](docs/SENSORS_AND_INPUTS.md) — ENV sensors, dimmer, warning inputs
- [docs/VALIDATION_STATE.md](docs/VALIDATION_STATE.md) — known-good behavior and remaining uncertainties
- [history/FINAL_SAVE_14_MANIFEST.md](history/FINAL_SAVE_14_MANIFEST.md) — final-version provenance and hashes
- [history/README.md](history/README.md) — historical checkpoints

## Development philosophy

Big Dirty was built by proving subsystems independently and integrating them in small steps:

1. keep known-good hardware timings and display initialization unless a test specifically requires a change
2. change one subsystem at a time
3. preserve a working baseline before refactoring
4. prefer measured behavior over assumptions
5. do not reorganize the sketch simply to make it look like a conventional desktop-code repository

## Privacy

This repository should contain project source, technical notes, schematics, datasheets, and test history only. Do not commit credentials, private network details, personal file paths, or other personally identifying information.

## License

No open-source license has been selected for this repository. Until one is explicitly added, normal copyright rules apply.
