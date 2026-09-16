# Changelog

This file records milestones supported by retained source, repository history, or confirmed project material.

## 2026-09-16 — Final Save 14 recovered

The exact source archive `BigDirty_UI_Main - Save 14.zip` was supplied and identified as the final version.

Compared with the 2026-04-07 GitHub source:

- 31 source files total
- 27 byte-for-byte identical
- 4 newer files restored:
  - `DTC_Manager.cpp`
  - `Display_ST7796S.cpp`
  - `Page_DTC.cpp`
  - `Page_Env.cpp`

Final-version behavior recovered:

- combined Mode 03 + Mode 07 DTC scanning
- duplicate suppression across stored/pending results
- Mode 04 clear followed by combined reread
- final DTC status wording
- DHT20 failure give-up behavior and UI fallback
- current/reference hardware-validity checks
- transmission-temperature failure fallback
- transmission-temperature interval increased to 1500 ms
- display SPI transaction ownership cleanup

The four updated root source files were verified against Save 14 by exact Git blob SHA.

See `history/FINAL_SAVE_14_MANIFEST.md`.

## 2026-09-16 — Warning RAW assets recovered

Recovered the exact five 96x72 warning-icon RAW files.

Confirmed:

- RGB565
- 16-bit
- little-endian
- 13,824 bytes per icon

The original `bowtie.raw` boot asset remains missing.

## 2026-09-16 — Repository documentation expansion

- Added comprehensive project documentation.
- Added architecture, build, pinout, UI, VPW, DTC, sensor, validation, and repository-map notes.
- Added SD-card asset requirements.
- Added roadmap and development notes.
- Preserved the 2026-04-07 integrated source as a historical Git checkpoint.
- Added a history area for earlier saved source snapshots.

## 2026-04-07 — Integrated firmware snapshot

Integrated source tree uploaded to GitHub.

That checkpoint included warning-input handling, analog dimmer/backlight control, live Mode 01 PID pages, Mode 03 DTC reading, Mode 04 DTC clearing, DHT20 ambient data, alternator-current calculation, transmission-temperature calculation, and redraw-on-change UI behavior.

It is superseded as the current source by recovered Save 14, but remains available in Git history.

## 2026-03-14 — Saved full-project snapshot

A retained `FULL_PROJECT_CODE.txt` snapshot captures an earlier point in development. At that point the page framework/custom fonts/boot/touch existed, while PID and DTC integration was substantially earlier.

The retained snapshot is archived under `history/`.

## Late 2025 — Teensy integration

Display, touch, and HC58 subsystems were proven individually and then brought onto the Teensy 4.1 application.

Known display baseline:

- ST7796S 480x320
- SPI 16 MHz, Mode 3
- MADCTL 0x28
- XPT2046 rotation 3

## 2025-10-19 — Four-PID HC58 golden timing logger

`Four_PIDs_at_once.ino` records the Arduino Uno validation build:

    Build_2025-10-19m_GoldenTiming_4PID_InsightLogger

It polls MAP, IAT, MAF, and TPS using:

- 8.3 ms post-TX
- 750 us EOD settle
- 13 ms drain window
- 5 ms poll gap
- 100 kHz SPI Mode 3

That work became the basis for the later Teensy SPI1 PID manager.
