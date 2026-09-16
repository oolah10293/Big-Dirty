# DTC Implementation

## Current feature set

- Mode 03 stored-code request
- positive-reply detection
- negative-reply detection
- P/C/B/U code decoding
- up to eight displayed codes
- curated code labels
- Mode 04 clear
- post-clear Mode 03 verification

The DTC page reads automatically when entered.

## Transport

DTC_Manager uses the HC58 on SPI1 at 100 kHz, Mode 3, with the same pins documented in PINOUT.md.

## Mode 03 request

Functional request:

    68 6A F1 03 00

Positive stored-code reply detector:

    48 6B 10 43

Following bytes are interpreted as two-byte DTC pairs until 00 00, buffer end, or eight codes.

## Negative reply

Recognized pattern:

    48 6B 10 7F 03 reason

Earlier HC58 testing specifically established handling of a negative response shaped like:

    7F 03 11

## Generic formatting

The first two bits select the system letter:

- 00 -> P
- 01 -> C
- 10 -> B
- 11 -> U

Unknown codes can still be displayed even when absent from the local label table.

## Curated labels

The current table covers practical MAF/MAP/IAT/ECT/TPS codes, upstream O2 and rich/lean codes, P0300-P0308 misfires, and several transmission speed/solenoid/TCC codes.

## Proven example

Project testing successfully used:

    P0102 - MAF LOW

as a real DTC-path validation case.

## Traffic heuristics

For an initial Mode 03 read with no recognized positive/negative response:

- 0 bytes -> error
- 0..20 bytes -> error
- 21..49 bytes -> ambiguous, retry once
- 50+ bytes -> treated as no codes

These are empirical heuristics.

## Mode 04 clear

Clear is only available in DTC_PRESENT.

Sequence:

1. send functional Mode 04
2. wait post-TX/EOD
3. listen/drain for roughly 5 seconds
4. wait another 250 ms
5. perform Mode 03 reread
6. update UI state

## Mode 07

Pending-code Mode 07 was validated during earlier HC58 work but is not part of the current integrated DTC UI.
