# Roadmap

Big Dirty has its recovered Save 14 baseline plus documented post-final work. The roadmap is preservation, verified improvements, and targeted cleanup—not reconstruction of missing application code.

## Preserve Save 14

- Keep Save 14 identifiable through `history/FINAL_SAVE_14_MANIFEST.md`.
- Keep the 2026-04-07 and March 2026 states available through history.
- Treat post-final fixes and experiments as explicit branches/commits rather than silently redefining Save 14.

## HC58 shared interrupt-driven transport — hardware validated

Interrupt operation and the production integration have now been proven on the actual hardware.

Protocol evidence:

- `0x12` masked interrupts produced zero edges
- `0x02` default interrupts produced reliable frame-completion events
- 100 consecutive IRQ-only MAP requests returned 100 valid replies with zero timeouts or fallback cleanup

Production integration:

- one shared HC58/SPI1 owner
- interrupt-driven RxFIFO service
- non-blocking PID and DTC state machines
- exact frame-content matching
- ENV -> PID1 transition fix
- reduced redundant display clearing
- HC58 service checkpoints during long display and SD drawing operations

Live Test 2 on 2026-10-24 completed a full warm-up through thermostat opening, repeated page cycling, engine start, and ignition-off without an observed functional problem. The log reached 16,545 completed frames with:

    RFO=0
    ERR=0
    TRUNC=0
    BUDGET=0
    STUCK=0
    CONFLICT=0

Live Test 1 had produced its first RxFIFO-overrun flag at 3,955 completed frames and 36 by 58,689 frames. The display-service revision therefore passed more than four times beyond the prior first-failure point with no overrun.

Evidence:

- `tests/HC58_INT_Diagnostic/RESULTS_2026-10-23.md`
- `tests/HC58_INT_Diagnostic/results/`
- `docs/HC58_SHARED_TRANSPORT_DESIGN.md`
- `docs/HC58_DISPLAY_SERVICE_TEST_RESULTS_2026-10-24.md`

The tested implementation remains on `test/hc58-shared-transport-live`. The remaining work is repository/release finalization rather than another functional transport rewrite.


## Recover the last missing SD asset

Still missing:

- `bowtie.raw` — 480x320 boot image

The five warning icons are already recovered and preserved.

When `bowtie.raw` is found:

1. preserve the exact file
2. record byte size and SHA-256
3. verify its byte order against `RawImage.cpp`
4. update the boot-loader documentation only after that evidence exists

## Optional ENV architecture cleanup

Final Save 14 still uses:

    Page_Env acquisition + rendering

Possible future structure:

    acquisition -> cached sensor state -> rendering

Only do this as a new tested revision. It is not needed to complete recovery of the final source.

## Known visual issue

The display has had an undesirable wipe/redraw direction in some transitions. It was intentionally tolerated rather than destabilize the known-good driver.

Only revisit this with a small isolated display test.

## Boot RAW byte-order cleanup

Warning-icon byte order is proven little-endian.

The remaining uncertainty is only `RawImage.cpp` versus the missing `bowtie.raw`. Do not infer the boot asset format from the warning assets and change known-working code without the original logo file.

## Lower-priority cleanup

After the final hardware state is backed up:

- centralize repeated UI quadrant helpers
- add explicit sensor-state structures
- record exact library versions from the build PC
- add a repeatable hardware regression checklist
