# J1850 VPW / MC68HC58 Notes

The MC68HC58 interfaces the Teensy to the GM J1850 VPW bus. The project validated the interface in standalone Arduino Uno sketches before porting it to Teensy 4.1 SPI1.

## Current Teensy wiring

| Signal | Pin |
|---|---:|
| INT | 0 |
| MISO | 1 |
| RST | 24 |
| CS | 25 |
| MOSI | 26 |
| SCK | 27 |

Integrated settings:

- SPI1
- 100 kHz
- MSB first
- Mode 3
- CS setup/hold 40 us

## Initialization

Established sequence:

    12 / 1A   CCR-first
    10 / C0   RENA + TENA

PID_Manager uses reset low 500 ms/high 5 ms. DTC_Manager in final Save 14 uses 40 ms low/120 ms high. Preserve that difference until tested.

## Functional Mode 01 request

PID_Manager emits:

    68 6A F1 01 PID

and searches for PCM replies beginning:

    48 6B 10 41 PID

## Integrated live-PID capture timing

| Parameter | Value |
|---|---:|
| POSTTX_MS | 9 ms |
| EOD_US | 750 us |
| QUIET_GAP_MS | 15 ms |
| MAX_CAPTURE_MS | 80 ms |
| POLL_GAP_MS | 10 ms |

RX is drained until traffic is quiet long enough or the capture window expires.

## Original golden Uno timing

`Four_PIDs_at_once.ino` records Build_2025-10-19m_GoldenTiming_4PID_InsightLogger:

| Parameter | Value |
|---|---:|
| Post-TX | 8.3 ms |
| EOD settle | 750 us |
| Drain window | 13.0 ms |
| Poll gap | 5 ms |

It polls MAP, IAT, MAF, and TPS.

## Current Mode 01 PID set

| PID | Meaning | Decode |
|---:|---|---|
| 04 | Calculated load | A * 100 / 255 |
| 06 | STFT bank 1 | (A - 128) * 100 / 128 |
| 07 | LTFT bank 1 | same |
| 08 | STFT bank 2 | same |
| 09 | LTFT bank 2 | same |
| 0B | MAP | A kPa |
| 0E | Timing | A / 2 - 64 degrees |
| 0F | IAT | A - 40 °C, converted to °F |
| 10 | MAF | (256A + B) / 100 g/s |
| 11 | TPS | A * 100 / 255 |
| 14 | O2 B1 | A * 5 mV |
| 15 | O2 B2 | A * 5 mV |

PID1 intentionally polls TPS more frequently. PID2 favors timing/load. PID3 round-robins the four fuel trims.

## Interrupt status

The INT signal exists but the integrated application does not use interrupt-driven RX. FIFO polling/draining is the established working behavior.

## Final diagnostic modes

Save 14 integrates:

- Mode 03 stored DTCs
- Mode 07 pending DTCs
- deduplication across Mode 03 + Mode 07
- Mode 04 clear
- combined Mode 03 + Mode 07 reread after clear

See [DTC.md](DTC.md) for the diagnostic state machine and traffic heuristics.

## Debugging rule

When VPW behavior is questionable, reproduce it with the smallest possible sketch first. Capture raw bytes and timing before changing the full UI application.
