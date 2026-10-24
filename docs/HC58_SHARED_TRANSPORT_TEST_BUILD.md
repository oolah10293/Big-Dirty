# HC58 Shared Transport Live Test Build

Build ID printed on USB Serial:

    BIGDIRTY_BUILD=HC58_INT_LIVE_TEST_1

Branch:

    test/hc58-shared-transport-live

## Scope

This is the first live-cluster integration of the interrupt-driven MC68HC58 work. It combines:

- one shared `HC58_Transport` hardware owner
- configuration byte `0x02` with IMSK cleared and IMOD still zero
- falling-edge INT handling on Teensy pin 0
- RFS/completion-code frame assembly
- continuous RxFIFO service on every page
- non-blocking PID request/echo/reply handling
- non-blocking Mode 03, Mode 07, and Mode 04 DTC operations
- the ENV -> PID1 transition fix that defers the destination page redraw until after the 200 ms touch/SPI guard

The display layout, supported PIDs, PID decode formulas, DTC labels, warning icons, ENV sensor calculations, and HC58 SPI speed/mode/timings are otherwise retained.

## Evidence used

The standalone diagnostic established on the actual vehicle hardware:

- 20/20 masked-poll requests succeeded with zero IRQ edges
- 20/20 IRQ-only requests succeeded with zero timeouts or fallback cleanup
- 100/100 IRQ-only MAP requests succeeded
- the 100-request run recorded 227 IRQ edges, zero timeouts, zero IRQ-log overflows, zero frame-list overflows, and zero fallback cleanup
- unrelated J1850 traffic can complete before or between Big Dirty's request echo and PCM response

Therefore this build identifies every frame from its bytes and completion code. It does not assume a fixed interrupt number or order.

## Shared transport behavior

`HC58_Transport.cpp` is now the only source file that owns:

- SPI1
- HC58 RST, CS, and INT pins
- the `0x02 / 0x1A` configuration exchange
- the retained `0x10 / 0xC0` post-configuration exchange
- RxFIFO reads and flushes
- frame completion boundaries
- transmitter ownership

The ISR only records an interrupt generation and pending flag. SPI and frame parsing remain in normal loop context.

The top-level loop services the transport both before and after `PageManager_update()` so full-page drawing and page-specific work do not leave completed frames unattended longer than necessary.

## PID behavior

The PID manager now uses these states:

    IDLE -> WAIT_TX_ECHO -> WAIT_RESPONSE

It waits for the exact transmitted request frame with `TMS=TX_OK` before starting the PCM response deadline. Background frames and lost-arbitration completions are ignored rather than mistaken for replies.

Initial test deadlines:

- transmit echo: 250 ms
- PCM response: 250 ms

These values are implementation choices awaiting live-cluster validation; they are not protocol constants.

## DTC behavior

Mode 03 and Mode 07 retain the existing timing intent as non-blocking deadlines:

- 250 ms pre-request hush
- 800 ms observation after each successful request transmission

Mode 04 retains:

- five-second listen period
- 250 ms post-clear delay
- combined Mode 03 + Mode 07 reread

The old 20/50 no-code traffic thresholds are preserved using the shared transport's nonempty-RxFIFO-item counter. Their equivalence under the new transport still requires vehicle validation.

## ENV -> PID1 mixed-page fix

A photographed failure showed PID1 values over stale ENV labels. PID1 labels are drawn only during the page's full `begin()` redraw, while later updates replace only values/units.

The page manager now performs transitions in this order:

1. queue the destination page
2. wait out the 200 ms touch/SPI guard
3. cancel the previous protocol owner if necessary
4. switch `currentPage`
5. run the destination page's complete `begin()` redraw
6. retain the existing 500 ms debounce before page updates begin

## USB Serial diagnostics

When USB Serial is connected, the build prints a summary every five seconds:

    HC58 IRQ=... FRAME=... RFO=... ERR=... TRUNC=... BUDGET=... STUCK=... CONFLICT=... OWNER=...

The first line identifies the build.

During normal operation, investigate any increase in:

- `RFO`
- `TRUNC`
- `BUDGET`
- `STUCK`
- `CONFLICT`

`ERR` counts HC58 frame completion codes with ERRF set and may require the surrounding vehicle/bus conditions for interpretation.

## Validation completed before packaging

Verified in the working environment:

- every source translation unit passes a C++17 syntax check against Arduino/Teensy API stubs
- the complete sketch links against those stubs
- the only warnings are two pre-existing unused parameters in `Display_Fonts.cpp`
- a fake-transport manager test passed for:
  - PID request transmission, exact request echo, response matching, and TPS decoding
  - unrelated-frame rejection
  - Mode 03 + Mode 07 DTC sequencing and P0102 decoding
  - Mode 04 clear timing and post-clear reread classification
  - transport ownership release

Not yet verified here:

- compilation with the user's installed Teensy 4.1 board package and exact external libraries
- live display, touch, PID, DTC, and long-run behavior on the truck

## Hardware test checklist

1. Compile for Teensy 4.1 with USB type Serial.
2. Upload with Tera Term and Arduino Serial Monitor closed.
3. Confirm normal boot, ENV values, warnings, and dimmer behavior.
4. Cycle ENV -> PID1 repeatedly and confirm that values and labels always belong to the same page.
5. Confirm PID1, PID2, and PID3 values continue updating.
6. Leave the display running on every page long enough to exercise background traffic.
7. Open Tera Term at 115200 if practical and capture the five-second HC58 summaries.
8. Read a known DTC if available; verify stored/pending behavior and duplicate suppression.
9. Test Mode 04 clear only when intentionally ready to clear codes.
10. During an extended run, check that RFO, truncation, service-budget, stuck-low, and owner-conflict counters do not increase.

Do not merge this branch into `main` until the live-cluster test is complete.
