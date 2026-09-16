# Changelog

This file reconstructs only milestones supported by retained source, repository history, or confirmed project notes. It intentionally avoids inventing missing version numbers.

## 2026-09-16 — Repository documentation expansion

- Added comprehensive project documentation.
- Added architecture, build, pinout, UI, VPW, DTC, sensor, validation, and repository-map notes.
- Added SD-card asset requirements.
- Added roadmap and development notes.
- Preserved the 2026-04-07 integrated source unchanged.
- Added a history area for earlier saved source snapshots.

## 2026-04-07 — Integrated firmware snapshot

Current source tree uploaded to GitHub.

This snapshot includes warning-input handling, analog dimmer/backlight control, live Mode 01 PID pages, Mode 03 DTC reading, Mode 04 DTC clearing, DHT20 ambient data, alternator-current calculation, transmission-temperature calculation, and redraw-on-change UI behavior.

## 2026-03-14 — Saved full-project snapshot

A retained FULL_PROJECT_CODE.txt snapshot captures an earlier point in development. At that point the page framework/custom fonts/boot/touch existed, while PID and DTC integration was substantially earlier.

The retained snapshot is archived under history/.

## Late 2025 — Teensy integration

Display, touch, and HC58 subsystems were proven individually and then brought onto the Teensy 4.1 application.

Known display baseline:

- ST7796S 480x320
- SPI 16 MHz, Mode 3
- MADCTL 0x28
- XPT2046 rotation 3

## 2025-10-19 — Four-PID HC58 golden timing logger

Four_PIDs_at_once.ino records the Arduino Uno validation build:

    Build_2025-10-19m_GoldenTiming_4PID_InsightLogger

It polls MAP, IAT, MAF, and TPS using:

- 8.3 ms post-TX
- 750 us EOD settle
- 13 ms drain window
- 5 ms poll gap
- 100 kHz SPI Mode 3

That work became the basis for the later Teensy SPI1 PID manager.
