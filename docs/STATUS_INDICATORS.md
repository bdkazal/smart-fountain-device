# Status Indicator LEDs

This document records the Smart Fountain V1 physical LED indicator behavior.

## Purpose

The status LEDs give quick local feedback without opening the Laravel dashboard or serial monitor.

Current indicators:

```text
Network/server status LED
Water safety status LED
```

## Pin map

| Indicator | GPIO | Behavior |
| --- | --- | --- |
| Network/server LED | GPIO23 | Shows Wi-Fi/Laravel connection state |
| Water safety LED | GPIO13 | Shows float switch / pump safety state |

Both LEDs are active HIGH in the current ESP32 DevKit V1 hardware config.

## Wiring

Use a current-limiting resistor for each LED.

```text
GPIO23 -> resistor -> Network LED -> GND
GPIO13 -> resistor -> Water Safety LED -> GND
```

Recommended resistor range:

```text
220R to 1kR
```

Do not connect an LED directly to an ESP32 GPIO without a resistor.

## Network/server LED behavior

The network/server LED combines Wi-Fi and Laravel/API status.

```text
fast blink = Wi-Fi disconnected or setup portal active
slow blink = Wi-Fi connected but Laravel/API offline
solid ON   = Wi-Fi connected and Laravel/API online
```

Implementation source:

```text
include/StatusIndicators.h
src/StatusIndicators.cpp
src/FountainApp.cpp
```

The LED is updated from the main runtime loop and also during Wi-Fi connection waiting, so the indicator continues to change while Wi-Fi is connecting.

## Water safety LED behavior

The water safety LED mirrors the current debounced water-low safety state.

```text
OFF      = water OK
solid ON = water low / pump protected
```

The water safety LED should turn ON when the float switch reports `water_low=true`. In this state, pump output is protected by firmware safety logic.

## Related water-low switch

Current water-low switch pin:

```text
GPIO32
```

Expected switch wiring:

```text
GPIO32 -> float switch -> GND
```

Expected input states:

```text
LOW  = water low / unsafe for pump
HIGH = water OK
```

The switch uses ESP32 internal pull-up and debounce logic.

## Test checklist

### Network/server LED

```text
[ ] Boot with Wi-Fi credentials available -> LED eventually solid ON
[ ] Stop Laravel/API while Wi-Fi remains connected -> LED slow blinks
[ ] Turn off Wi-Fi/router or use invalid Wi-Fi -> LED fast blinks
[ ] Enter setup portal mode -> LED fast blinks
```

### Water safety LED

```text
[ ] Float switch reports water OK -> LED OFF
[ ] Float switch reports water low -> LED solid ON
[ ] While water low, pump ON request is blocked/forced OFF
[ ] When water returns OK, LED turns OFF after debounce
```

## Confirmed status

Current hardware test result:

```text
[x] Network/server LED works
[x] Water safety LED works
```
