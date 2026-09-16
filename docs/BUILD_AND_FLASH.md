# Build and Flash

## Target

- Board: Teensy 4.1
- Framework: Arduino / Teensy core
- Main sketch: BigDirty_UI_Main.ino
- Authoritative source: BigDirty_UI_Main - Save 14

All project .cpp and .h files are intended to remain in the same Arduino sketch folder as the .ino.

## Libraries

Provided by the Teensy / Arduino environment:

- Arduino
- SPI
- SD
- Wire

External:

- XPT2046_Touchscreen
- DHT20 library compatible with the API used in Page_Env.cpp

The DHT20 source expects `DHT20`, `begin()`, `read()`, `getTemperature()`, `getHumidity()`, and `DHT20_OK`.

The exact installed library versions were not recoverable with confidence from the repository. Record them from the known-good build PC when available.

## Historical .ino warning

`Four_PIDs_at_once.ino` is an Arduino Uno reference sketch, not the application entry point.

If an Arduino IDE workflow tries to compile both .ino files together, build from a working copy that excludes the historical Uno sketch from the active sketch folder while preserving it in Git.

## SD card

The application calls `SD.begin(BUILTIN_SDCARD)`. The boot logo and warning graphics are loaded from the SD-card root.

The five warning RAW files have been recovered. The original `bowtie.raw` boot asset has not.

See ../sdcard/README.md.

## Regression checks after flashing

1. Boot logo appears with correct colors/orientation.
2. Logo fades and ENV appears.
3. One physical tap advances exactly one page.
4. ENV values update without label corruption.
5. Missing/invalid ENV sensors fall back to `---` without locking the UI.
6. PID1/PID2/PID3 populate from VPW.
7. DTC page scans stored and pending codes.
8. Duplicate Mode 03/Mode 07 codes appear only once.
9. A known DTC can be read.
10. Clear works only when intentionally pressed.
11. Post-clear reread returns the expected no-code state.
12. Warning icons match input states.
13. Backlight follows the vehicle dimmer.
14. Cycling pages does not create stale touch artifacts.

## Serial debugging

`setup()` starts Serial at 115200.

Save 14 can print:

    DHT20 Lost: Screen updated to ---

after three failed DHT reads in one ENV visit.

For VPW debugging, prefer temporary raw-frame diagnostics or `Four_PIDs_at_once.ino` rather than permanent heavy printing inside time-sensitive loops.

## Before low-level changes

Back up the exact final firmware and SD assets before changing ST7796S init, SPI ownership, SPI modes/frequencies, HC58 timing, raw-image byte order, touch calibration, or analog scaling.
