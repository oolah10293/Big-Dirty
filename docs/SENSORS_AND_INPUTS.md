# Sensors and Vehicle Inputs

## ENV values

| Quadrant | Value |
|---|---|
| Q1 | Ambient temperature °F |
| Q2 | Humidity % |
| Q3 | Alternator current A |
| Q4 | Transmission temperature °F |

The authoritative Save 14 source performs sensor reads inside `Page_Env.cpp`. A later preferred architecture can move acquisition outside the page, but that refactor is not part of the final firmware.

## Final update intervals

| Channel | Interval |
|---|---:|
| DHT20 temperature/humidity | 1000 ms |
| Alternator current | 250 ms |
| Transmission temperature | 1500 ms |

On entry to ENV, Save 14 resets all four displayed values to `---` and forces the timing state so fresh readings can replace them immediately.

## DHT20

- SDA 18
- SCL 19
- nominal update interval 1000 ms
- Wire timeout configured to 1000 microseconds by the final source

Temperature is converted from Celsius to Fahrenheit and temperature/humidity are rounded to whole display values.

### Failure handling

Save 14 adds:

- `dhtFailCount`
- `dhtGaveUp`

After three failed reads during one ENV page visit:

1. both DHT values are forced to `---`
2. `dhtGaveUp` is set
3. further DHT polling stops for that page visit

Entering ENV again clears the give-up state and allows another attempt.

This avoids repeatedly stalling the UI on a missing or failed I2C sensor.

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

Then:

- magnitude below 2 A -> 0 A
- negative result -> 0 A
- display rounded/clamped to 0..999

### Final hardware-validity checks

Save 14 refuses the current calculation when:

- MCP1525 raw reading is below 500
- CT VREF raw reading is below 2482
- CT VREF raw reading is above 3723

The comments identify the VREF range as approximately 2.0 to 3.0 V. A failed current read displays `---`.

The retained interface-board documents should be used for physical protection/filter component details.

## Transmission temperature

Final source:

- sender pin 14
- MCP1525 monitor 17
- 2.500 V reference
- 2.2 kOhm modeled pull-up to 3.3 V
- update interval **1500 ms**

Resistance:

    R = 2200 * Vnode / (3.3 - Vnode)

Steinhart-Hart coefficients:

- A = 1.443335e-03
- B = 1.722714e-04
- C = 3.974502e-07

Save 14 rejects invalid conditions including:

- bad reference
- sender raw <= 0
- calculated Vnode <= 0.001 V
- Vnode >= 3.290 V
- nonpositive resistance
- nonpositive inverse temperature

A failed transmission-temperature read displays `---`.

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

## Possible future acquisition architecture

A cleaner separation would be:

    hardware sampling -> validated/cached engineering values -> page rendering

That remains a potential refactor only. Save 14 is the authoritative final firmware and intentionally remains unchanged.
