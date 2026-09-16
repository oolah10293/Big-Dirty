# Big Dirty

Big Dirty is a standalone Teensy 4.1 instrument-cluster / vehicle-data display built around a 480x320 ST7796S TFT, XPT2046 touch controller, and Motorola MC68HC58 J1850 VPW interface.

The integrated firmware talks directly to a 2005-era GM 8.1L PCM over VPW, displays live OBD data, reads several local sensors, handles vehicle warning inputs, and provides a touch-driven DTC read/clear screen.

The repository deliberately keeps the Arduino sketch and all .cpp/.h modules in the sketch folder. That matches the known working development layout and avoids reorganizing a functioning embedded project for cosmetic reasons.

## Current repository state

The current integrated source snapshot was uploaded on **2026-04-07**. It contains:

- known-good ST7796S display initialization and custom drawing primitives
- XPT2046 touch input with one-event-per-tap edge detection
- SD-card boot-logo and warning-icon support
- five runtime pages after the boot logo
- live Mode 01 VPW PID polling through the MC68HC58
- stored DTC read through Mode 03
- DTC clear through Mode 04 followed by an automatic reread
- ambient temperature and humidity from a DHT20
- alternator-current measurement
- transmission-temperature measurement
- five warning inputs
- analog dimmer-controlled backlight

The project was substantially functional in real use. A few known architectural and display issues are intentionally documented rather than silently "fixed" in this archive. See [docs/VALIDATION_STATE.md](docs/VALIDATION_STATE.md).

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
| DTC | stored trouble-code list and clear control | | | |

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

The display driver intentionally preserves the proven settings:

- 480x320 landscape
- SPI at 16 MHz
- MSB first
- SPI Mode 3
- ST7796S MADCTL = 0x28
- RGB565 / 16-bit color
- XPT2046 rotation = 3

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

The DTC page currently supports:

- Mode 03 stored-code request
- positive reply parsing
- negative-reply detection
- generic P/C/B/U DTC formatting
- a curated label table for common engine/transmission codes
- up to eight displayed codes
- Mode 04 clear
- automatic Mode 03 reread after clear

A P0102 MAF-low code was successfully used during project testing.

See [docs/DTC.md](docs/DTC.md).

## Local sensor processing

The ENV page currently contains the sensor-acquisition code used by the April 2026 snapshot:

- DHT20: 1 second update interval
- alternator current: 250 ms update interval
- transmission temperature: 250 ms update interval
- Teensy ADC: 12-bit with 16-sample averaging

The later architectural preference was to make Page_Env display-only and move acquisition outside the page. The repository snapshot predates that cleanup, so the working source is preserved as-is and the migration is documented instead of guessed.

See [docs/SENSORS_AND_INPUTS.md](docs/SENSORS_AND_INPUTS.md).

## SD card assets

The firmware expects RAW RGB565 assets in the SD-card root. The binary assets are not currently present in this repository.

Required by the current code:

- /bowtie.raw — 480x320 boot image
- brake.raw — 96x72
- checkengine.raw — 96x72
- highbeam.raw — 96x72
- 4WD.raw — 96x72
- seatbelt.raw — 96x72

See [sdcard/README.md](sdcard/README.md) before regenerating or replacing them.

## Building

This is an Arduino/Teensy sketch layout.

Required target:

- Teensy 4.1

External libraries used by the current source:

- XPT2046_Touchscreen
- a DHT20 library exposing DHT20, begin(), read(), getTemperature(), getHumidity(), and DHT20_OK

The normal Teensy core supplies SPI, SD, Wire, Arduino timing/GPIO, and built-in SD-card support.

See [docs/BUILD_AND_FLASH.md](docs/BUILD_AND_FLASH.md).

## Repository guide

- [DEVELOPMENT.md](DEVELOPMENT.md) — architecture and change discipline
- [CHANGELOG.md](CHANGELOG.md) — recoverable project milestones
- [ROADMAP.md](ROADMAP.md) — remaining cleanup and optional work
- [docs/PROJECT_CONTEXT.md](docs/PROJECT_CONTEXT.md) — goals and design decisions
- [docs/REPOSITORY_MAP.md](docs/REPOSITORY_MAP.md) — source-file roles
- [docs/PINOUT.md](docs/PINOUT.md) — pin assignments
- [docs/UI_AND_DISPLAY.md](docs/UI_AND_DISPLAY.md) — display, touch, pages, warnings
- [docs/VPW_HC58.md](docs/VPW_HC58.md) — VPW interface and Mode 01 details
- [docs/DTC.md](docs/DTC.md) — Mode 03/04 behavior
- [docs/SENSORS_AND_INPUTS.md](docs/SENSORS_AND_INPUTS.md) — ENV sensors, dimmer, warning inputs
- [docs/VALIDATION_STATE.md](docs/VALIDATION_STATE.md) — known-good behavior and known divergences
- [history/README.md](history/README.md) — archived checkpoints

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
