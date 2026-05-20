#pragma once

// Prototype hardware pin configuration for the Biztola Smart Fountain.
//
// Current V1 hardware test scope:
// - Pump controlled through LR7843 MOSFET module
// - Normal switch / float switch connected to GPIO and GND for water safety
// - COB and RGB physical drivers disabled until their hardware is finalized
//
// IMPORTANT:
// - ESP32 GPIO pins must never receive 12V.
// - Pump power must go through the 12V supply and MOSFET module.
// - ESP32 GND, 12V supply GND, MOSFET GND, and buck GND must be common.
// - Keep the pump in water before ON testing.

#define SMART_FOUNTAIN_HARDWARE_ENABLED 1

// Pump output through LR7843 MOSFET module.
// Wiring: GPIO5 -> LR7843 signal/input.
// ACTIVE_HIGH means higher PWM duty turns the pump MOSFET/load ON.
//
// Calibration note from real pump test:
// - 10% is the minimum target running duty.
// - 11%, 15%, and 100% are useful comparison points for this pump.
// - A 700ms startup boost helps the pump start reliably before dropping to 10%.
#define PUMP_OUTPUT_PIN 5
#define PUMP_OUTPUT_ACTIVE_HIGH 1
#define PUMP_PWM_ENABLED 1
#define PUMP_PWM_CHANNEL 1
#define PUMP_PWM_FREQUENCY 1000
#define PUMP_PWM_RESOLUTION_BITS 8
#define PUMP_PWM_MIN_DUTY_PERCENT 10
#define PUMP_PWM_MAX_DUTY_PERCENT 100
#define PUMP_STARTUP_BOOST_DUTY_PERCENT 100
#define PUMP_STARTUP_BOOST_MS 700

// Float switch / temporary normal switch input.
// Wiring: GPIO1 -> switch -> GND.
// Internal pull-up keeps the pin HIGH when the switch is open.
// ACTIVE_LOW means closed switch reads LOW and becomes water_low=true.
#define WATER_LEVEL_SWITCH_PIN 1
#define WATER_LEVEL_SWITCH_ACTIVE_LOW 1
#define WATER_LEVEL_SWITCH_USE_PULLUP 1
#define WATER_LEVEL_SWITCH_DEBOUNCE_MS 150

// COB physical output disabled for now.
// The 5W COB LEDs need current-limited driving before MOSFET/PWM testing.
#define COB_PWM_PIN -1
#define COB_PWM_CHANNEL 0
#define COB_PWM_FREQUENCY 5000
#define COB_PWM_RESOLUTION_BITS 8

// RGB physical output disabled for now.
// Later choose addressable RGBIC/WS2812B/WS2811 or analog RGB PWM driver.
#define RGB_HARDWARE_TYPE_NONE 0
#define RGB_HARDWARE_TYPE RGB_HARDWARE_TYPE_NONE
#define RGB_DATA_PIN -1
#define RGB_RED_PIN -1
#define RGB_GREEN_PIN -1
#define RGB_BLUE_PIN -1
