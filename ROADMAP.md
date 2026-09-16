# Roadmap

Big Dirty now has its authoritative final firmware, Save 14. The roadmap is therefore preservation, verification, and optional targeted cleanup—not reconstruction of missing application code.

## Preserve Save 14

- Treat the current root source as the final firmware baseline.
- Keep the 2026-04-07 and March 2026 states available through history rather than overwriting them.
- Use `history/FINAL_SAVE_14_MANIFEST.md` to verify source provenance.

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

## Optional HC58 transport unification

`PID_Manager.cpp` and `DTC_Manager.cpp` each contain HC58 initialization, SPI cycle, and timing code.

A shared transport module could reduce duplication, but only after capturing current live-PID and Mode 03/07/04 behavior.

## Known visual issue

The display has had an undesirable wipe/redraw direction in some transitions. It was intentionally tolerated rather than destabilize the known-good driver.

Only revisit this with a small isolated display test.

## Boot RAW byte-order cleanup

Warning-icon byte order is now proven little-endian.

The remaining uncertainty is only `RawImage.cpp` versus the missing `bowtie.raw`. Do not infer the boot asset format from the warning assets and change known-working code without the original logo file.

## Lower-priority cleanup

After the final hardware state is backed up:

- centralize repeated UI quadrant helpers
- centralize HC58 constants if testing supports it
- add explicit sensor-state structures
- record exact library versions from the build PC
- add a repeatable hardware regression checklist
