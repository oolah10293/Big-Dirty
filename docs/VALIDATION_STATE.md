# Validation State and Known Divergences

This document separates the authoritative final source from historical checkpoints, proven subsystem behavior, and remaining uncertainties.

## Authoritative final source

**BigDirty_UI_Main - Save 14** is the final firmware snapshot.

Recovered: 2026-09-16

Archive SHA-256:

    796d88659c82b9c2126da6ee5c1ea51ffc89b3903e8b878baae54b5e1e9f73a0

Compared with the earlier 2026-04-07 GitHub source:

- 31 source files total
- 27 identical
- 4 changed
  - DTC_Manager.cpp
  - Display_ST7796S.cpp
  - Page_DTC.cpp
  - Page_Env.cpp

The recovered Save 14 source remains the archival baseline. Current `main` intentionally differs in `PageManager.cpp` by one post-final transition fix added on 2026-10-19.

See [../history/FINAL_SAVE_14_MANIFEST.md](../history/FINAL_SAVE_14_MANIFEST.md).

## Strongly established baselines

### Display

- Teensy 4.1
- ST7796S 480x320
- 16 MHz SPI
- Mode 3
- MADCTL 0x28
- RGB565

Save 14 additionally makes display SPI transaction ownership explicit and avoids the earlier nested-transaction pattern.

### Touch

The XPT2046 path and page-change reset/debounce logic were developed specifically to stop repeated/ghost page changes from one press.

On 2026-10-19, an observed ENV -> PID1 transition showed valid PID1 values over stale ENV labels. Repository review found no label mix-up in the page source; instead, PID1 labels depend on the single full redraw in `Page_PID1_begin()`. Current `main` therefore defers that destination-page `begin()` until the 200 ms touch/SPI guard has expired. This fix is hardware-validation pending.

### HC58 live data

HC58 polling was validated first in standalone sketches. `Four_PIDs_at_once.ino` preserves the 2025-10-19 golden MAP/IAT/MAF/TPS logger. The Teensy PID manager expands this into three live-data pages.

### DTC

The project established Mode 03 read and Mode 04 clear in hardware. P0102 was a successful real code-reading test.

The final Save 14 implementation also integrates Mode 07 pending-code scanning into the DTC page and merges/deduplicates Mode 03 + Mode 07 results.

### ENV sensor failure behavior

Save 14 adds explicit fallback behavior for missing/invalid DHT20, current-sensor, reference, and transmission-temperature inputs. Invalid values display `---`.

### Warning graphics

The exact five 96x72 warning RAW assets have been recovered.

Each is:

- RGB565
- 16-bit
- 13,824 bytes
- little-endian

This confirms the byte order used by `Overlay_Warnings.cpp`.

### Project-level status

The user identified Save 14 as the final version. Treat it as the baseline to preserve, not as a draft awaiting retrospective cleanup.

## Present in the final source

- boot logo path
- page navigation
- warning overlay
- dimmer backlight
- ENV acquisition/rendering with failure handling
- live PID pages
- combined Mode 03 + Mode 07 DTC scan
- Mode 04 DTC clear
- post-clear combined reread
- HC58 polling

## Architectural note: ENV ownership

Final source:

    Page_Env owns DHT20 and analog acquisition

A later cleanup concept separates acquisition from rendering. Because Save 14 is now known to be the final firmware, that concept should be treated as optional future work—not as a missing piece of the recovered source.

## Architectural note: duplicated HC58 ownership

`PID_Manager.cpp` and `DTC_Manager.cpp` each define HC58 pins, configure SPI1, reset/init the chip, and implement `SPIcycle()`. They also use different reset timing.

This is a refactor candidate, not evidence that the final build is incomplete.

## HC58 interrupt

INT is wired/configured, but the application polls/drains FIFO. Interrupt-driven RX is not required.

## Known visual issue

A screen wipe/redraw direction remained visually annoying in later use. It was tolerated because the display otherwise worked.

## RAW byte order

### Warning icons — resolved

Recovered warning files are explicitly little-endian RGB565.

`Overlay_Warnings.cpp` reads:

    low byte first
    high byte second

which matches the recovered assets.

### Boot logo — still unresolved

`RawImage.cpp` names its first byte `hi` and second byte `lo`, despite comments describing the source as little-endian.

The original 480x320 `bowtie.raw` is still missing, so the boot-logo path should remain unchanged until that exact known-good asset is recovered.

## Asset recovery state

Recovered and preserved in `sdcard/warning-icons-rgb565.zip`:

- `brake.raw`
- `checkengine.raw`
- `highbeam.raw`
- `4WD.raw`
- `seatbelt.raw`

Still missing:

- `bowtie.raw`

## Historical source is not current source

`history/FULL_PROJECT_CODE_2026-03-14.txt` predates Save 14 and remains useful only for archaeology.

The old 2026-04-07 GitHub state is preserved naturally in repository history.

## Review items before any future modification

- DTC traffic-length heuristics
- separate HC58 resets in PID vs DTC managers
- fullscreen boot-logo RAW endianness
- optional ENV acquisition/render split
- visible wipe direction
