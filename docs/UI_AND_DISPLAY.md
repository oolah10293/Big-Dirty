# UI, Display, and Touch

## Display baseline

Final Save 14 ST7796S initialization:

- SPI 16 MHz
- MSB first
- Mode 3
- unlock F0 C3 / F0 96
- MADCTL command 36 data 28
- pixel format command 3A data 55
- X 0..479, Y 0..319
- RGB565

Treat this as a known-good baseline.

## SPI transaction ownership in Save 14

`Display_ST7796S.cpp` now has explicit helpers:

- `displaySPI_begin()`
- `displaySPI_end()`

The begin helper forces the touch controller CS high before opening the display SPI transaction.

`setAddrWindow()` assumes the caller already owns the transaction. It no longer opens/closes its own nested SPI transaction.

`drawPixel_FAST()`, `fillScreen_FAST()`, and `fillRect_FAST()` each own one complete transaction around the operation.

The fill routines restore the default full-screen address window before ending.

This is a meaningful final-version cleanup compared with the older April source.

## Drawing primitives

`Display_ST7796S.cpp` provides `setAddrWindow()`, `drawPixel_FAST()`, `fillScreen_FAST()`, `fillRect_FAST()`, `setBacklightPWM()`, and dimmer/backlight update functions.

The UI stays practical without a framebuffer by repainting only changed regions.

## Fonts

`Display_Fonts.cpp` contains project-specific 5x7 digits, uppercase letters, percent, degree, slash, and mixed value/unit helpers.

## Page map

PageManager registers:

1. PAGE_BOOTLOGO
2. PAGE_ENV
3. PAGE_PID1
4. PAGE_PID2
5. PAGE_PID3
6. PAGE_DTC

BootLogo must remain enum value 0.

## Navigation

Boot:

- starts dark
- clears black
- reveals `/bowtie.raw`
- waits roughly 3 seconds unless touched
- fades 32 steps at 25 ms/step
- switches to ENV

Runtime:

    ENV -> PID1 -> PID2 -> PID3 -> DTC -> ENV

A tap anywhere advances, except the DTC clear-button region.

## Touch

XPT2046:

- CS 22
- IRQ 21
- rotation 3
- raw calibration 200..3800
- screen map 480x320

`readTouch()` is edge-triggered. PageManager resets the edge detector and adds transition blocking/debounce to prevent one press from skipping pages.

## Common quadrant layout

- left X 0..239
- right X 240..479
- top Y about 10..119
- bottom Y about 120..230
- warning strip begins around Y 242

## DTC page wording

Final Save 14 status text:

- SCANNING CODES
- NO DIAGNOSTIC CODES
- CODE READ ERROR
- CODE REQUEST ERROR

The DTC list can contain merged Mode 03 stored and Mode 07 pending codes.

## Warning overlay

Icons are 96x72 at Y 242.

Current source maps:

| X | Asset |
|---:|---|
| 0 | brake.raw |
| 96 | checkengine.raw |
| 192 | highbeam.raw |
| 288 | 4WD.raw |
| 384 | seatbelt.raw |

The overlay caches previous states and redraws only changed slots.

The recovered warning assets confirm low-byte-first little-endian RGB565.

## Boot RAW byte-order uncertainty

The warning-loader byte order is resolved.

The only remaining RAW-format uncertainty is the fullscreen boot path: `RawImage.cpp` names its first byte high and second low while its comments describe little-endian input.

Do not change it until the original working `bowtie.raw` is recovered.

## Known visual issue

A later annoyance was the direction of a visible wipe/redraw during some transitions. It was left alone because the display otherwise worked. Treat it as cosmetic, not a reason to rewrite the driver wholesale.
