# Build and Flash

## Target

- Board: Teensy 4.1
- Framework: Arduino / Teensy core
- Main sketch: BigDirty_UI_Main.ino

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

The DHT20 source expects DHT20, begin(), read(), getTemperature(), getHumidity(), and DHT20_OK.

The exact installed library versions were not recoverable with confidence from the repository. Record them from the known-good build PC when available.

## Historical .ino warning

Four_PIDs_at_once.ino is an Arduino Uno reference sketch, not the application entry point.

If an Arduino IDE workflow tries to compile both .ino files together, build from a working copy that excludes the historical Uno sketch from the active sketch folder while preserving it in Git.

## SD card

The application calls SD.begin(BUILTIN_SDCARD). The boot logo and warning graphics are loaded from the SD-card root.

See ../sdcard/README.md.

## Regression checks after flashing

1. Boot logo appears with correct colors/orientation.
2. Logo fades and ENV appears.
3. One physical tap advances exactly one page.
4. ENV values update without label corruption.
5. PID1/PID2/PID3 populate from VPW.
6. DTC page behaves correctly on a no-code bus.
7. A known DTC can be read.
8. Clear works only when intentionally pressed.
9. Warning icons match input states.
10. Backlight follows the vehicle dimmer.
11. Cycling pages does not create stale touch artifacts.

## Serial debugging

setup() starts Serial at 115200.

For VPW debugging, prefer temporary raw-frame diagnostics or Four_PIDs_at_once.ino rather than permanent heavy printing inside time-sensitive loops.

## Before low-level changes

Back up the exact working firmware and SD assets before changing ST7796S init, SPI modes/frequencies, HC58 timing, raw-image byte order, touch calibration, or analog scaling.
