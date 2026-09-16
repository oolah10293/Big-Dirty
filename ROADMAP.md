# Roadmap

Big Dirty is already a substantially functional project. The roadmap is mostly preservation, verification, and targeted cleanup rather than feature expansion.

## Preserve the actual latest working state

- Keep the 2026-04-07 GitHub snapshot as a known checkpoint.
- If a newer exact working sketch is recovered from the programming PC, compare it file-by-file before replacing anything.
- Preserve recovered SD-card RAW assets once their exact working versions are found.

## Move ENV acquisition out of Page_Env

Target:

    acquisition -> cached sensor state -> display page

The current source still reads DHT20 and analog sensors from Page_Env.cpp. Migrate only with hardware testing rather than recreating missing code from memory.

## Unify HC58 transport ownership

PID_Manager.cpp and DTC_Manager.cpp each contain their own HC58 initialization, SPI cycle, and timing code.

A shared transport module could reduce duplication, but only after capturing current live-PID and DTC read/clear behavior.

## Asset preservation

Recover and commit the exact known-good SD files:

- bowtie.raw
- brake.raw
- checkengine.raw
- highbeam.raw
- 4WD.raw
- seatbelt.raw

## Known visual issue

The display has had an undesirable wipe/redraw direction in some transitions. It was intentionally tolerated rather than destabilize the known-good driver.

Only revisit this with a small isolated display test.

## RAW byte-order cleanup

Current warning icons are read low-byte then high-byte. The fullscreen RawImage loader names the first byte high while its comments describe little-endian data.

Resolve this against the actual working RAW assets before changing code.

## Optional diagnostic expansion

Mode 07 pending-code behavior was validated during earlier HC58 work. The integrated DTC UI currently uses Mode 03 stored codes and Mode 04 clear only.

Possible future work:

- add Mode 07 pending-code display
- preserve separate stored vs pending status
- expand the curated code-description table only as useful

## Lower-priority cleanup

After the working hardware state is backed up:

- centralize repeated UI quadrant helpers
- centralize HC58 constants
- add explicit sensor-state structures
- record exact library versions from the build PC
- add a repeatable hardware regression checklist
