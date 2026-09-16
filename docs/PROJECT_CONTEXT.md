# Project Context

Big Dirty is a standalone vehicle instrument / diagnostic display built around a Teensy 4.1. It reads the vehicle directly instead of depending on a phone, aftermarket head unit, or another computer.

The project combines GM J1850 VPW data, local analog/I2C sensors, warning-light inputs, touch UI, SD-card graphics, and diagnostic trouble-code access. The target PCM is a 2005-era GM 8.1L controller.

## Core design goals

1. Standalone operation whenever the vehicle is powered.
2. Direct VPW communication through the MC68HC58 rather than an ELM327-style external adapter.
3. Compact four-value pages instead of displaying every available PID.
4. Local acquisition of ambient temperature, humidity, alternator current, and transmission temperature.
5. Basic DTC read/clear on the same screen.
6. Persistent warning-state behavior and dimmer-following backlight.
7. Modular troubleshooting: display, touch, VPW, sensor, and warning subsystems were developed independently before integration.

## Why Teensy 4.1

The application needs main SPI for display/touch, separate SPI1 for HC58, several analog inputs, I2C, built-in SD, GPIO for warning inputs, PWM backlight control, and enough CPU for custom graphics/polling.

## Why the low-level display code stayed

The ST7796S was one of the difficult bring-up pieces. Once a reliable init/orientation/SPI combination was found, it became a baseline. The project therefore kept a small custom display layer rather than replacing it with a larger graphics stack.

## Why the HC58 code is timing-heavy

The VPW path was tuned empirically. Post-transmit delay, EOD settle, drain windows, CS setup/hold, and poll gaps are tested hardware parameters, not formatting details.

## UI philosophy

Runtime pages share:

- black background
- aqua values/labels
- two-by-two data quadrants
- warning strip along the bottom
- tap to advance

The DTC page is the exception because it needs a variable list and clear control.

## Scope boundary

This repository is for the touchscreen/Teensy cluster and its directly related hardware, source, test history, and reference documents. Unrelated projects and personal data do not belong here.
