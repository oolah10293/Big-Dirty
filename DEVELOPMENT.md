# Development Notes

## Working rule

Big Dirty is an embedded hardware project with several independently sensitive subsystems. The safest development model is incremental:

- preserve a known-good baseline
- change one thing
- test it on hardware
- keep the change if it solves the target problem
- avoid unrelated cleanup in the same revision

The authoritative baseline is now **BigDirty_UI_Main - Save 14**, identified as the final version.

This repository intentionally keeps the .ino, .cpp, and .h files together in the Arduino sketch directory. Do not move them into src/ unless the build workflow is deliberately changed and retested.

## High-level architecture

    BigDirty_UI_Main.ino
      |
      +-- Display_ST7796S / Display_Fonts / RawImage
      +-- Touch_XPT2046
      +-- PageManager
      |     +-- Page_BootLogo
      |     +-- Page_Env
      |     +-- Page_PID1
      |     +-- Page_PID2
      |     +-- Page_PID3
      |     +-- Page_DTC
      |
      +-- Overlay_Warnings
      +-- PID_Manager
      +-- DTC_Manager
      +-- PinConfig

SPI0 is shared by the display and touch controller. The MC68HC58 is isolated onto SPI1. The Teensy built-in SD interface supplies the RAW UI assets.

## Main loop responsibilities

The top-level loop:

1. samples the five warning inputs
2. updates the active page
3. redraws warning icons when required, except during the boot-logo page
4. updates backlight PWM from the vehicle dimmer input

## Page lifecycle

PageManager stores begin and update function pointers for every page.

Transitions are queued rather than performed immediately. A transition switches on the following PageManager update, runs the new page begin function, resets touch edge state, and blocks touch briefly after the change.

There is also a 500 ms page-change debounce before page updates resume. This behavior was introduced to stop held/stale touch state from skipping multiple pages or creating transition artifacts.

## Redraw strategy

The UI avoids full-screen repainting for every value change.

ENV and PID pages:

- draw the page once
- cache the displayed text
- compare new values against cached text
- clear only the old value/unit block
- redraw only the changed value

The warning overlay likewise tracks previous icon states and redraws only when a warning changes.

## Display driver

Do not casually change the established display baseline:

- SPI 16 MHz
- MSB first
- Mode 3
- MADCTL 0x28
- RGB565
- 480x320 landscape

Save 14 uses explicit `displaySPI_begin()` / `displaySPI_end()` ownership. `setAddrWindow()` assumes the transaction is already open. Preserve that model unless a controlled display/touch regression test justifies changing it.

## Touch

Touch is edge-triggered. `readTouch()` produces one event on the rising edge of a physical press. `Touch_reset()` clears the software edge detector after page changes.

Current calibration:

- raw minimum 200
- raw maximum 3800
- screen map 0..479 by 0..319
- rotation 3

## HC58 ownership

Save 14 retains separate HC58 setup/state inside PID_Manager and DTC_Manager. Both use SPI1 and can initialize/reset the HC58.

That is functional but duplicated ownership. A future cleanup could create one HC58 transport layer shared by live PID and DTC code. Do not perform that refactor casually: it changes timing/state ownership in a bus interface that was difficult to stabilize.

## ENV ownership

Final `Page_Env.cpp` both acquires and renders DHT20/current/transmission-temperature data.

An optional future architecture is:

    sensor acquisition -> cached values -> Page_Env rendering

Because Save 14 is now recovered, this is a potential refactor rather than reconstruction of missing code.

## Blocking work

Most UI behavior is non-blocking, but DTC traffic contains deliberate long windows. Mode 03 and Mode 07 each drain for hundreds of milliseconds, and Mode 04 clear listens for several seconds before a combined reread.

Do not aggressively shorten these timings without hardware validation.

## Reference sketch

`Four_PIDs_at_once.ino` is a historical Arduino Uno timing logger, not the Teensy application entry point.

It records the earlier golden chain:

- post-TX 8.3 ms
- EOD settle 750 us
- drain 13 ms
- inter-poll gap 5 ms
- HC58 SPI 100 kHz, Mode 3
- CCR-first initialization
- RENA + TENA

## Change checklist

Before changing a subsystem, record the exact source revision, connected hardware, expected behavior, observed behavior, what changed, and whether the previous behavior still works.

For VPW changes, capture raw frames whenever possible. For display changes, preserve the current init sequence and test orientation, fill direction, touch alignment, and SD RAW images separately.
