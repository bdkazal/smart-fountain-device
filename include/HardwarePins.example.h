#pragma once

// Copy this file to include/HardwarePins.h when real hardware is ready.
// Do not commit include/HardwarePins.h if it contains prototype-specific wiring.
//
// V1 hardware is intentionally guarded. Outputs stay software-only until
// SMART_FOUNTAIN_HARDWARE_ENABLED is set to 1 and safe pins are configured.

#define SMART_FOUNTAIN_HARDWARE_ENABLED 0

// Pump output through LR7843 MOSFET module.
// ACTIVE_HIGH means higher PWM duty turns the MOSFET/load ON.
#define PUMP_OUTPUT_PIN -1
#define PUMP_OUTPUT_ACTIVE_HIGH 1
#define PUMP_PWM_ENABLED 1
#define PUMP_PWM_CHANNEL 1
#define PUMP_PWM_FREQUENCY 1000
#define PUMP_PWM_RESOLUTION_BITS 8
#define PUMP_PWM_MIN_DUTY_PERCENT 10
#define PUMP_PWM_MAX_DUTY_PERCENT 100
#define PUMP_STARTUP_BOOST_DUTY_PERCENT 100
#define PUMP_STARTUP_BOOST_MS 700

// Float switch / test switch input.
// Suggested first prototype pin: GPIO1.
// Wiring: GPIO -> switch -> GND, using ESP32 internal pull-up.
// ACTIVE_LOW means closed switch reads LOW and becomes water_low=true.
#define WATER_LEVEL_SWITCH_PIN -1
#define WATER_LEVEL_SWITCH_ACTIVE_LOW 1
#define WATER_LEVEL_SWITCH_USE_PULLUP 1
#define WATER_LEVEL_SWITCH_DEBOUNCE_MS 150

// COB light PWM output.
// TBD: choose final ESP32-C3 GPIO and current-limited driver circuit.
#define COB_PWM_PIN -1
#define COB_PWM_CHANNEL 0
#define COB_PWM_FREQUENCY 5000
#define COB_PWM_RESOLUTION_BITS 8

// RGB output.
// V1 placeholder. Later choose RGB type:
// - addressable LED strip/module, or
// - separate PWM channels for R/G/B, or
// - external LED driver.
#define RGB_HARDWARE_TYPE_NONE 0
#define RGB_HARDWARE_TYPE RGB_HARDWARE_TYPE_NONE
#define RGB_DATA_PIN -1
#define RGB_RED_PIN -1
#define RGB_GREEN_PIN -1
#define RGB_BLUE_PIN -1
