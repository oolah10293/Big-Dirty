# HC58 Shared Transport Live Test 2 — Display Service / RFO Fix

Build ID printed on USB Serial:

    BIGDIRTY_BUILD=HC58_INT_LIVE_TEST_2

Branch:

    test/hc58-shared-transport-live

## Why this build exists

Live Test 1 passed the visible functional checks during a full warm-up:

- normal boot
- correct ENV and PID pages
- responsive TPS and plausible values from the other PIDs
- repeated page cycling without mixed ENV/PID1 labels and values
- engine start and key-off without an observed failure

The USB diagnostic log nevertheless recorded 36 HC58 RxFIFO-overrun completion flags by 58,689 completed frames. The other transport-integrity counters stayed at zero:

    RFO=36
    ERR=0
    TRUNC=0
    BUDGET=0
    STUCK=0
    CONFLICT=0

The first overrun appeared after 3,955 completed frames. This build addresses the most credible software cause: long, uninterrupted display and SD drawing operations preventing normal-loop RxFIFO service.

## Scope

This revision keeps the proven interrupt transport, PID/DTC state machines, PID formulas, page layout, warning inputs, and 200 ms ENV -> PID1 redraw guard.

It changes only display-work scheduling and redundant page clearing.

## Page-clear policy

The four gauge pages share the same layout:

- ENV
- PID1
- PID2
- PID3

A transition within that group no longer performs a full-screen clear. The destination page clears each of its four quadrants exactly once, with an HC58 service checkpoint between quadrants.

A transition crossing between the gauge group and Boot or DTC performs one chunked full-screen black clear in PageManager. Because the background is then already black, the destination gauge page draws its values and labels without clearing the four quadrants again.

The redundant setup-time black clear was removed. Boot retains its own deliberate black clear immediately before the boot image.

DTC entry uses the PageManager full clear and draws its initial state without immediately clearing the list and button regions a second time. Later DTC state changes still clear the areas they replace.

## Display service checkpoints

### Solid fills

`fillRect_FAST()` divides large fills into strips capped at approximately 2,048 pixels. At 16 MHz RGB565, each uninterrupted data burst is roughly 2 ms or less before `HC58Transport::service()` is called.

`fillScreen_FAST()` now uses that same chunked rectangle path.

### Fonts and individual pixels

`drawPixel_FAST()` services the HC58 whenever INT is physically asserted or after 32 individual pixel writes. Existing font and mixed-text drawing therefore gain service opportunities without changing their appearance or layout.

### Boot image

The 480x320 RAW boot image is now read and sent one complete row at a time. The byte interpretation is unchanged from the recovered firmware. HC58 is serviced after every row.

### Warning icons

The 96x72 warning icons are now read and sent one row at a time. Their established little-endian RGB565 interpretation is preserved. HC58 is serviced after every row.

## Warning overlay cache

After a known full-screen black clear, PageManager tells the warning overlay that all five icon slots are physically empty. The next normal update draws only warnings that are actually active instead of clearing five already-black slots.

Gauge-to-gauge transitions preserve the warning bar and its cached state.

## USB diagnostics

The five-second diagnostic line now includes the active page number:

    HC58 IRQ=... FRAME=... RFO=... ERR=... TRUNC=... BUDGET=... STUCK=... CONFLICT=... OWNER=... PAGE=...

Page IDs:

- 0 Boot
- 1 ENV
- 2 PID1
- 3 PID2
- 4 PID3
- 5 DTC

This makes any future counter increase easier to correlate with page activity.

## Validation target

Repeat the Live Test 1 conditions:

1. boot normally
2. start the engine
3. cycle through all pages repeatedly
4. run through a full warm-up, including thermostat opening
5. exercise DTC read if appropriate
6. turn the key off and restart if practical
7. capture the complete USB diagnostic log

Required transport result:

    RFO=0
    TRUNC=0
    BUDGET=0
    STUCK=0
    CONFLICT=0

Also confirm:

- ENV/PID1 labels and values never mix
- warning icons still draw and clear correctly
- boot image appearance and byte order are unchanged
- all PID pages remain responsive
- DTC behavior remains correct

## Build verification before packaging

Completed in the working environment:

- every translation unit passed a C++17 compile against Arduino/Teensy API stubs
- the complete sketch linked against those stubs
- the linked stub executable ran through setup and one loop pass
- the only compiler warnings were the two pre-existing unused parameters in `Display_Fonts.cpp`

The actual Teensy 4.1 board-package compile and hardware behavior remain the authoritative tests.


## Hardware validation result — 2026-10-24

**PASS.**

The vehicle test completed a full warm-up through thermostat opening, repeated page cycling, engine start, and an ignition-off tail without an observed functional problem.

The captured run reached 16,545 completed HC58 frames with all transport-failure counters still zero:

    RFO=0
    ERR=0
    TRUNC=0
    BUDGET=0
    STUCK=0
    CONFLICT=0

Live Test 1 had recorded its first `RFO` at 3,955 completed frames. Live Test 2 therefore ran more than four times beyond that first-failure point without another overrun.

Detailed hardware results and hashes:

- `docs/HC58_DISPLAY_SERVICE_TEST_RESULTS_2026-10-24.md`
- `tests/HC58_INT_Diagnostic/results/live_test_2_2026-10-24.txt.gz`

No additional functional transport change is indicated by this captured test.
