# Sensors and Vehicle Inputs

## ENV values

| Quadrant | Value |
|---|---|
| Q1 | Ambient temperature °F |
| Q2 | Humidity % |
| Q3 | Alternator current A |
| Q4 | Transmission temperature °F |

The April 2026 source performs sensor reads inside Page_Env.cpp. A later preferred architecture moves acquisition outside the page.

## DHT20

- SDA 18
- SCL 19
- update interval 1000 ms

Temperature is converted from Celsius to Fahrenheit and temperature/humidity are rounded to whole display values.

## Alternator-current channel

Pins:

- MCP1525 monitor 17
- sensor VREF 15
- sensor VOUT 16

ADC:

- 12-bit
- 16-sample averaging

Reference normalization:

    voltsPerCount = 2.500 / rawMcp
    vref = rawVref * voltsPerCount
    vout = rawVout * voltsPerCount

Conversion:

    deltaV = vout - vref
    amps = -(deltaV / 0.004)
    amps = amps * 1.220

Then values below 2 A magnitude are forced to zero, negative results are forced to zero, and the displayed value is rounded/clamped to 0..999.

The retained interface-board documents should be used for physical protection/filter component details.

## Transmission temperature

Current source:

- sender pin 14
- MCP1525 monitor 17
- 2.500 V reference
- 2.2 kOhm modeled pull-up to 3.3 V

Resistance:

    R = 2200 * Vnode / (3.3 - Vnode)

Steinhart-Hart coefficients:

- A = 1.443335e-03
- B = 1.722714e-04
- C = 3.974502e-07

Update interval: 250 ms.

## Dimmer / backlight

DIMMER_SENSE pin 20.

Divider model:

- top 38.1 kOhm
- bottom 11.87 kOhm

Behavior:

- source below 1 V -> full bright
- source above 13 V -> full bright
- 1..13 V -> normalized and raised to the fourth power for PWM curve

Backlight:

- pin 23
- 8-bit PWM
- 20 kHz

The boot page owns brightness until startup is complete.

## Warning inputs

| Warning | Pin | On when |
|---|---:|---|
| Seat belt | 2 | HIGH |
| Brake | 3 | HIGH |
| High beam | 4 | HIGH |
| Check engine | 6 | LOW |
| 4WD | 7 | LOW |

The warning overlay is suppressed during the boot-logo page.

## Preferred acquisition architecture

Long-term separation:

    hardware sampling -> validated/cached engineering values -> page rendering

The current source is preserved because it is a real working checkpoint; cleanup should be a tested revision, not a retrospective rewrite.
