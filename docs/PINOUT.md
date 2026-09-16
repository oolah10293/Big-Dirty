# Pinout

This reflects the current 2026-04-07 source plus the established Teensy bus pins.

## SPI0: ST7796S + XPT2046

| Function | Pin |
|---|---:|
| SPI0 MOSI | 11 |
| SPI0 MISO | 12 |
| SPI0 SCK | 13 |
| TFT CS | 10 |
| TFT DC | 9 |
| TFT RST | 8 |
| TFT backlight PWM | 23 |
| Touch CS | 22 |
| Touch IRQ | 21 |

## SPI1: MC68HC58

| Signal | Pin |
|---|---:|
| INT | 0 |
| MISO | 1 |
| RST | 24 |
| CS | 25 |
| MOSI | 26 |
| SCK | 27 |

The integrated code configures INT but does not depend on interrupt-driven RX. FIFO data is polled/drained over SPI.

SPI1 settings:

- 100 kHz
- MSB first
- Mode 3
- 40 us CS setup
- 40 us CS hold

## DHT20 / I2C

Wire default pins:

| Function | Pin |
|---|---:|
| SDA | 18 |
| SCL | 19 |

## ENV analog channels

| Function | Pin |
|---|---:|
| Transmission-temperature sender node | 14 |
| Current-sensor VREF | 15 |
| Current-sensor VOUT | 16 |
| MCP1525 2.5 V reference monitor | 17 |

ADC configuration is 12-bit with 16-sample averaging.

## Dimmer

| Function | Pin |
|---|---:|
| DIMMER_SENSE | 20 |

Software models a 38.1 kOhm / 11.87 kOhm external divider.

## Warning inputs

| Signal | Pin | Active state |
|---|---:|---|
| Seat belt | 2 | HIGH |
| Brake | 3 | HIGH |
| High beam | 4 | HIGH |
| Check engine | 6 | LOW |
| 4WD | 7 | LOW |

## Source hierarchy for wiring disagreements

Use this order before rewiring:

1. actual installed hardware / continuity
2. current known-working source
3. Interface board connections.xlsx
4. Interface board schematic.pdf
5. historical notes

Pin assignments changed during bring-up, so old test sketches should not automatically be treated as final wiring.
