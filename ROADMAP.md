# Roadmap

Big Dirty has its recovered Save 14 baseline plus documented post-final work. The roadmap is preservation, verified improvements, and targeted cleanup—not reconstruction of missing application code.

## Preserve Save 14

- Keep Save 14 identifiable through `history/FINAL_SAVE_14_MANIFEST.md`.
- Keep the 2026-04-07 and March 2026 states available through history.
- Treat post-final fixes and experiments as explicit branches/commits rather than silently redefining Save 14.

## HC58 shared interrupt-driven transport — active design

Interrupt operation has been proven on the actual hardware:

- `0x12` masked interrupts produced zero edges
- `0x02` default interrupts produced reliable frame-completion events
- 100 consecutive IRQ-only MAP requests returned 100 valid replies with zero timeouts or fallback cleanup

Evidence:

- `tests/HC58_INT_Diagnostic/RESULTS_2026-10-23.md`
- `tests/HC58_INT_Diagnostic/results/`

Current design:

- `docs/HC58_SHARED_TRANSPORT_DESIGN.md`
- `design/HC58_Transport_API.h`

Next implementation work:

1. add the shared transport as the only HC58/SPI1 owner
2. add continuous frame dispatch
3. convert PID requests to a non-blocking request/echo/reply state machine
4. convert DTC read/clear to non-blocking states while preserving current classification behavior
5. remove duplicated manager-level HC58 code and timing-based drain loops
6. validate live gauges, page changes, Mode 03/07, Mode 04, and long-run error counters

No production package should be generated until an implementation revision is explicitly approved for hardware testing.

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
