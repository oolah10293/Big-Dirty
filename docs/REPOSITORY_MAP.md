# Repository Map

## Application

### BigDirty_UI_Main.ino
Top-level setup/loop: initializes hardware/managers, samples warning inputs, updates pages/overlays/backlight.

## Display/UI infrastructure

### Display_ST7796S.cpp / .h
Low-level ST7796S driver and backlight control.

### Display_Fonts.cpp / .h
Project bitmap fonts and value/unit helpers.

### Touch_XPT2046.cpp / .h
Touch init, calibration mapping, one-event-per-press behavior.

### RawImage.cpp / .h
Fullscreen 480x320 RAW loader used by boot.

### Overlay_Warnings.cpp / .h
Five 96x72 warning icons with state-change redraw.

### PageManager.cpp / .h
Page IDs, begin/update dispatch, queued transitions, touch reset, debounce.

## Pages

- Page_BootLogo.cpp/.h — boot image/fade
- Page_Env.cpp/.h — ENV UI plus current snapshot sensor acquisition
- Page_PID1.cpp/.h — TPS/IAT/MAP/MAF
- Page_PID2.cpp/.h — load/timing/O2 B1/O2 B2
- Page_PID3.cpp/.h — STFT/LTFT banks 1 and 2
- Page_DTC.cpp/.h — DTC list, clear button, navigation

## Vehicle-data managers

### PID_Manager.cpp / .h
HC58 SPI1 setup, Mode 01 request/capture/decoding, page-specific polling, four display slots.

### DTC_Manager.cpp / .h
Mode 03/04 traffic, response classification, DTC decoding/labels, state machine.

## Hardware configuration

### PinConfig.cpp / .h
Shared display/touch/dimmer/warning pins.

HC58 and ENV analog pins are still locally defined in the current managers/pages.

## Historical/reference code

### Four_PIDs_at_once.ino
Arduino Uno golden-timing HC58 logger. Reference code, not the Teensy application.

### history/
Older retained source checkpoints.

## Reference documents

- Interface board schematic.pdf
- Interface board connections.xlsx
- CT Datasheet.pdf
- DHT20 Temp-humidity datasheet.pdf
- TS76_CoolantSensor_Curve_final.pdf

## External runtime assets

Not currently committed:

- bowtie.raw
- brake.raw
- checkengine.raw
- highbeam.raw
- 4WD.raw
- seatbelt.raw

See ../sdcard/README.md.
