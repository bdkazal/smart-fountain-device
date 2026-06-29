# Hardware Pins and Device I/O

This document records the current Smart Fountain ESP32 hardware map.

## Current pin map

| Role | GPIO | Current behavior |
| --- | --- | --- |
| Pump output | GPIO25 | Digital output, active HIGH |
| COB output | GPIO26 | Digital output, active HIGH |
| RGB / NeoPixel data | GPIO27 | NeoPixel data output |
| Water-low switch | GPIO32 | Digital input, active LOW, debounced |
| Network/server status LED | GPIO17 | Digital output, active HIGH |
| Water safety status LED | GPIO16 | Digital output, active HIGH |
| Pump local button | GPIO19 | Digital input, active LOW, debounced |
| COB local button | GPIO18 | Digital input, active LOW, debounced |
| Wi-Fi reset button | GPIO23 | Hold to GND for setup/reset flow |
| RTC SDA | GPIO21 | I2C SDA for DS3231/HW-111 |
| RTC SCL | GPIO22 | I2C SCL for DS3231/HW-111 |

## Physical layout rule

The ESP32 DevKit V1 / ESP32-WROOM-32 controller board is organized like this for the veroboard build:

```text
Left side GPIOs  = fountain/RJ45/external-load side
Right side GPIOs = local controller-box switches, LEDs, RTC
```

Current grouping:

```text
Left side:
GPIO25 = pump MOSFET control
GPIO26 = COB MOSFET control
GPIO27 = RGB / NeoPixel data
GPIO32 = float switch input

Right side:
GPIO19 = pump local button
GPIO18 = COB local button
GPIO23 = Wi-Fi reset/setup button
GPIO17 = network/server status LED
GPIO16 = water safety status LED
GPIO21 = RTC SDA
GPIO22 = RTC SCL
```

## Output behavior

### Pump

```text
pin: GPIO25
active: HIGH
state ON: HIGH
state OFF: LOW
```

The pump must be protected by water-low safety.

If `water_low=true`:

```text
pump forced OFF
pump ON command ignored
state payload should report pump OFF
```

### COB light

```text
pin: GPIO26
active: HIGH
state ON: HIGH
state OFF: LOW
```

Current V1 behavior is ON/OFF. Dimming may be added later with PWM, but should not be mixed into the schedule/timeline verification work.

### RGB / NeoPixel

```text
pin: GPIO27
count: 12
color order: GRB
max brightness: 160
```

Supported effects currently include:

```text
solid
breathing
slow_rainbow
warm_glow
water_shimmer
night_mode
```

## Indicator behavior

### Network/server status LED

```text
pin: GPIO17
active: HIGH
```

Expected LED states:

```text
fast blink = Wi-Fi disconnected or setup portal active
slow blink = Wi-Fi connected but Laravel/API offline
solid ON   = Wi-Fi connected and Laravel/API online
```

### Water safety status LED

```text
pin: GPIO16
active: HIGH
```

Expected LED states:

```text
OFF      = water OK
solid ON = water low / pump protected
```

Both status LEDs must be wired with current-limiting resistors:

```text
GPIO -> resistor -> LED -> GND
```

Recommended resistor range:

```text
220R to 1kR
```

## Input behavior

### Water-low switch

```text
pin: GPIO32
active: LOW
```

Expected state:

```text
LOW  = water low / unsafe for pump
HIGH = water OK
```

Serial logs may show:

```text
Water-low switch changed: WATER OK
Water-low switch changed: WATER LOW
```

### Pump local button

```text
pin: GPIO19
active: LOW
```

Behavior:

```text
press -> toggle pump
if requested ON and water_low=true -> ignore and keep pump OFF
```

### COB local button

```text
pin: GPIO18
active: LOW
```

Behavior:

```text
press -> toggle COB light
```

### Wi-Fi reset/setup button

```text
pin: GPIO23
active: LOW
hold time: 3 seconds
```

Behavior:

```text
hold to GND for 3 seconds -> clear saved Wi-Fi -> start setup hotspot
```

## RTC module

The current firmware supports DS3231/HW-111 RTC.

```text
SDA: GPIO21
SCL: GPIO22
```

RTC rules:

```text
RTC stores UTC time.
Laravel server_time_utc is canonical when reachable.
RTC is updated only when drift is above threshold.
DeviceClock applies timezone_offset_minutes for local schedule interpretation.
```

## Safe boot rule

At boot, all hardware outputs are forced OFF first.

Expected log:

```text
All outputs forced OFF: safe boot hardware default
Output state untrusted: safe boot OFF is hardware-only until cached or fresh config loads
```

Important:

```text
Safe boot OFF must not be pushed to Laravel until a trusted cached or fresh config is loaded.
```

## Hardware still needing real-world confirmation

```text
[ ] actual pump MOSFET/relay electrical behavior under load
[ ] actual COB driver behavior and brightness/PWM plan
[ ] final water-level sensor/switch wiring
[ ] network/server LED visibility and blink timing on GPIO17
[ ] water safety LED visibility on GPIO16
[ ] long-run NeoPixel power stability
[ ] waterproofing and enclosure cable routing
```

## Change control rule

Do not change pin assignments inside unrelated feature work.

Pin changes should be their own commit/PR because they affect real hardware behavior.
