#pragma once

// Copy this file to include/HardwarePins.h when real hardware is ready.
// Do not commit include/HardwarePins.h if it contains prototype-specific wiring.
//
// V1 hardware is intentionally guarded. Outputs stay software-only until
// SMART_FOUNTAIN_HARDWARE_ENABLED is set to 1 and safe pins are configured.

#define SMART_FOUNTAIN_HARDWARE_ENABLED 0

// Pump output through LR7843 MOSFET module.
// ACTIVE_HIGH means higher PWM duty turns the MOSFET/load ON.
// 0% is OFF. 1-9% are clamped to 10%. 10-100% use direct PWM duty.
// Low-speed assist can periodically kick the pump at very low speeds so it
// keeps running without using a constant stronger duty.
#define PUMP_OUTPUT_PIN -1
#define PUMP_OUTPUT_ACTIVE_HIGH 1
#define PUMP_PWM_ENABLED 1
#define PUMP_PWM_CHANNEL 1
#define PUMP_PWM_FREQUENCY 20000
#define PUMP_PWM_RESOLUTION_BITS 8
#define PUMP_PWM_MIN_DUTY_PERCENT 10
#define PUMP_PWM_MAX_DUTY_PERCENT 100
#define PUMP_STARTUP_BOOST_DUTY_PERCENT 100
#define PUMP_STARTUP_BOOST_MS 700
#define PUMP_LOW_ASSIST_ENABLED 1
#define PUMP_LOW_ASSIST_MAX_SPEED_PERCENT 10
#define PUMP_LOW_ASSIST_DUTY_PERCENT 100
#define PUMP_LOW_ASSIST_KICK_MS 80
#define PUMP_LOW_ASSIST_INTERVAL_MS 2000

// Float switch / test switch input.
// Suggested ESP32 DevKit V1 pin: GPIO32.
// Wiring: GPIO -> switch -> GND, using ESP32 internal pull-up.
// ACTIVE_LOW means closed switch reads LOW and becomes water_low=true.
#define WATER_LEVEL_SWITCH_PIN -1
#define WATER_LEVEL_SWITCH_ACTIVE_LOW 1
#define WATER_LEVEL_SWITCH_USE_PULLUP 1
#define WATER_LEVEL_SWITCH_DEBOUNCE_MS 150

// Status indicator LEDs.
// Suggested ESP32 DevKit V1 right-side pins:
// - network/server status LED: GPIO17
// - water safety status LED: GPIO16
// Wiring for ACTIVE_HIGH: GPIO -> resistor -> LED -> GND.
// Set to -1 to disable.
#define NET_STATUS_LED_PIN -1
#define NET_STATUS_LED_ACTIVE_HIGH 1
#define WATER_SAFETY_LED_PIN -1
#define WATER_SAFETY_LED_ACTIVE_HIGH 1
#define NET_STATUS_FAST_BLINK_MS 250
#define NET_STATUS_SLOW_BLINK_MS 1000

// Local controller-box buttons.
// Suggested ESP32 DevKit V1 right-side pins:
// - pump local button: GPIO19
// - COB local button: GPIO18
// - Wi-Fi reset/setup button: GPIO23
// Wiring: GPIO -> button -> GND, using ESP32 internal pull-up.
#define LOCAL_PUMP_BUTTON_PIN -1
#define LOCAL_COB_BUTTON_PIN -1
#define WIFI_RESET_BUTTON_PIN 23
#define LOCAL_BUTTONS_ACTIVE_LOW 1
#define LOCAL_BUTTON_DEBOUNCE_MS 50

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
