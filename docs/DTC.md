# DTC Implementation

## Final feature set

Save 14 implements one combined diagnostic view built from:

- Mode 03 stored-code request
- Mode 07 pending-code request
- merge + duplicate suppression across both services
- positive-reply detection
- negative-reply detection
- P/C/B/U code decoding
- up to eight unique displayed codes
- curated code labels
- Mode 04 clear
- post-clear combined Mode 03 + Mode 07 verification

The DTC page scans automatically when entered.

## Transport

`DTC_Manager.cpp` uses the HC58 on SPI1 at 100 kHz, Mode 3, with the same pins documented in PINOUT.md.

DTC-manager HC58 reset remains:

- 40 ms low
- 120 ms high

This differs from PID_Manager and is preserved because it is part of the final working source.

## Functional requests

Common functional header:

    68 6A F1

Read-service payloads:

    03 00   stored codes
    07 00   pending codes

Clear payload:

    04 00

### Positive replies

Mode 03:

    48 6B 10 43

Mode 07:

    48 6B 10 47

The final implementation generalizes the positive service ID as `service + 0x40`.

### Negative replies

Recognized form:

    48 6B 10 7F service reason

Earlier HC58 testing specifically established handling of:

    7F 03 11

The final parser applies the same negative-reply structure to both Mode 03 and Mode 07.

## Merge and duplicate suppression

Save 14 maintains both:

- `rawDtcList[MAX_CODES][8]`
- formatted `dtcList[MAX_CODES][48]`

Before adding a code, `hasRawCode()` checks whether the raw P/C/B/U code is already present. That prevents a DTC returned by both Mode 03 and Mode 07 from occupying two display slots.

Maximum display list size remains eight unique codes.

## Generic formatting

The first two bits select the system letter:

- 00 -> P
- 01 -> C
- 10 -> B
- 11 -> U

Unknown codes can still be displayed even when absent from the local label table.

## Curated labels

The final table covers practical MAF/MAP/IAT/ECT/TPS codes, upstream O2 and rich/lean codes, P0300-P0308 misfires, and several transmission speed/solenoid/TCC codes.

## Proven example

Project testing successfully used:

    P0102 - MAF LOW

as a real DTC-path validation case.

## Timing

| Parameter | Final value |
|---|---:|
| POSTTX_US | 8300 us |
| EOD_US | 750 us |
| DRAIN_MS_READ | 800 ms |
| DRAIN_MS_CLEAR | 400 ms |
| PRETX_HUSH_READ_MS | 250 ms |
| POST_CLEAR_DELAY_MS | 250 ms |
| CLEAR_LISTEN_TOTAL_MS | 5000 ms |

## Traffic heuristics

The final code still uses empirical traffic length to distinguish a live bus/no codes from a failed read when neither service returns a recognized reply.

Thresholds:

- `TRAFFIC_ERROR_MAX = 20`
- `TRAFFIC_NONE_MIN = 50`

Initial-read behavior may retry Mode 03 and Mode 07 once when traffic is ambiguous.

After a clear, the no-code classification is intentionally more permissive: traffic above `TRAFFIC_ERROR_MAX` can be accepted as no codes.

Treat these as hardware-tuned heuristics, not protocol constants.

## Mode 04 clear

Clear is only available in `DTC_STATE_PRESENT`.

Sequence:

1. send functional Mode 04
2. wait post-TX/EOD
3. repeatedly drain/listen for about 5 seconds
4. wait another 250 ms
5. set post-clear read mode
6. perform combined Mode 03 + Mode 07 read
7. update UI state

## Final UI wording

`Page_DTC.cpp` in Save 14 uses:

- `SCANNING CODES`
- `NO DIAGNOSTIC CODES`
- `CODE READ ERROR`
- `CODE REQUEST ERROR`

The clear button remains visible only when one or more codes are present.
