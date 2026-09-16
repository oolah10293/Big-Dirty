# Validation State and Known Divergences

This separates proven/present behavior from cleanup targets.

## Strongly established baselines

### Display

- Teensy 4.1
- ST7796S 480x320
- 16 MHz SPI
- Mode 3
- MADCTL 0x28
- RGB565

Display and touch were tested independently before full integration.

### Touch

The XPT2046 path and page-change reset/debounce logic were developed specifically to stop repeated/ghost page changes from one press.

### HC58 live data

HC58 polling was validated first in standalone sketches. Four_PIDs_at_once.ino preserves the 2025-10-19 golden MAP/IAT/MAF/TPS logger. The Teensy PID manager expands this into three live-data pages.

### DTC

Mode 03 read and Mode 04 clear were validated. P0102 was a successful real code-reading test.

### Warning graphics

The exact five 96x72 warning RAW assets have now been recovered.

Each is:

- RGB565
- 16-bit
- 13,824 bytes
- little-endian

This confirms the byte order used by `Overlay_Warnings.cpp`.

### Project-level status

By later use, the cluster was largely functional. Remaining work was tweaks/cleanup rather than an unproven design.

## Present in the 2026-04-07 source

- boot logo
- page navigation
- warning overlay
- dimmer backlight
- ENV sensor acquisition/rendering
- live PID pages
- DTC page
- HC58 polling
- DTC clear

These still deserve regression testing after a long gap.

## Known divergence: ENV architecture

Later intent:

    sensors update independently
    Page_Env only renders cached values

Current source:

    Page_Env owns DHT20 and analog acquisition

Preserved rather than guessed.

## Known divergence: duplicated HC58 ownership

PID_Manager and DTC_Manager each define HC58 pins, configure SPI1, reset/init the chip, and implement SPIcycle(). They also use different reset timing.

This is a refactor candidate, not an emergency bug.

## HC58 interrupt

INT is wired/configured, but the working application polls/drains FIFO. Interrupt-driven RX is not required.

## Known visual issue: wipe direction

A screen wipe/redraw direction remained visually annoying in later use. It was left alone because the display otherwise worked.

## RAW byte order

### Warning icons — resolved

The recovered warning files include metadata explicitly identifying them as little-endian RGB565.

`Overlay_Warnings.cpp` reads:

    low byte first
    high byte second

which is correct for these recovered assets.

### Boot logo — still unresolved

`RawImage.cpp` names its first byte `hi` and second byte `lo`, despite comments calling the source little-endian.

Because the original 480x320 `bowtie.raw` is still missing, the boot-logo path should remain unchanged until that exact asset is recovered.

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

The March 2026 archived full-project text predates the April integrated build. It is for archaeology, not wholesale restoration.

## Review items before modification

- DTC traffic-length heuristics
- separate HC58 resets in PID vs DTC managers
- fullscreen boot-logo RAW endianness
- ENV sensor ownership
- visible wipe direction
