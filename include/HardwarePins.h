#pragma once

// Hardware pin configuration for the Biztola Smart Fountain on ESP32 DevKit V1 / ESP32-WROOM-32.
//
// Current hardware test scope:
// - Pump controlled through LR7843 MOSFET module
// - Normal switch / float switch connected to GPIO and GND for water safety
// - COB controlled as ON/OFF through a MOSFET/driver signal
// - 12 LED WS2812B / NeoPixel ring or strip controlled through one data pin
//
// IMPORTANT:
// - ESP32 GPIO pins must never receive 12V or 5V.
// - Pump/COB/NeoPixel power must go through the external supply and driver/load path.
// - ESP32 GND, power-supply GND, MOSFET GND, buck GND, and NeoPixel GND must be common.
// - Keep the pump in water before ON testing.
// - Do not connect LEDs directly to ESP32 GPIO except the NeoPixel data input.
// - Avoid ESP32 flash pins GPIO6-GPIO11 and boot strapping pins for safety inputs.

#define SMART_FOUNTAIN_HARDWARE_ENABLED 1

// Pump output through LR7843 MOSFET module.
// Wiring: GPIO25 -> LR7843 signal/input.
// ACTIVE_HIGH means GPIO HIGH turns the pump MOSFET/load ON.
#define PUMP_OUTPUT_PIN 25
#define PUMP_OUTPUT_ACTIVE_HIGH 1
#define PUMP_PWM_ENABLED 0
#define PUMP_PWM_CHANNEL 1
#define PUMP_PWM_FREQUENCY 20000
#define PUMP_PWM_RESOLUTION_BITS 8
#define PUMP_PWM_MIN_DUTY_PERCENT 10
#define PUMP_PWM_MAX_DUTY_PERCENT 100
#define PUMP_STARTUP_BOOST_DUTY_PERCENT 100
#define PUMP_STARTUP_BOOST_MS 0
#define PUMP_LOW_ASSIST_ENABLED 0
#define PUMP_LOW_ASSIST_MAX_SPEED_PERCENT 10
#define PUMP_LOW_ASSIST_DUTY_PERCENT 100
#define PUMP_LOW_ASSIST_KICK_MS 0
#define PUMP_LOW_ASSIST_INTERVAL_MS 2000

// Float switch / temporary normal switch input.
// Wiring: GPIO32 -> switch -> GND.
// Internal pull-up keeps the pin HIGH when the switch is open.
// ACTIVE_LOW means closed switch reads LOW and becomes water_low=true.
#define WATER_LEVEL_SWITCH_PIN 32
#define WATER_LEVEL_SWITCH_ACTIVE_LOW 1
#define WATER_LEVEL_SWITCH_USE_PULLUP 1
#define WATER_LEVEL_SWITCH_DEBOUNCE_MS 150

// COB output through a MOSFET/driver signal.
// Wiring: GPIO26 -> COB MOSFET signal/input.
// ACTIVE_HIGH means GPIO HIGH turns the COB output ON.
// Current hardware is ON/OFF only. No brightness/PWM dimming yet.
#define COB_OUTPUT_PIN 26
#define COB_OUTPUT_ACTIVE_HIGH 1
#define COB_PWM_PIN -1
#define COB_PWM_CHANNEL 0
#define COB_PWM_FREQUENCY 5000
#define COB_PWM_RESOLUTION_BITS 8

// RGB / accent light hardware type.
// Laravel still sends this as rgb_light: enabled, brightness_percent, color, effect.
// Firmware now renders that logical rgb_light state to a 12 LED WS2812B/NeoPixel output.
#define RGB_HARDWARE_TYPE_NONE 0
#define RGB_HARDWARE_TYPE_ANALOG_PWM 1
#define RGB_HARDWARE_TYPE_NEOPIXEL 2
#define RGB_HARDWARE_TYPE RGB_HARDWARE_TYPE_NEOPIXEL

// WS2812B / NeoPixel ring or strip.
// Wiring:
// NeoPixel 5V  -> external 5V supply +
// NeoPixel GND -> external 5V supply -
// ESP32 GND    -> same common ground
// GPIO27       -> 330R resistor -> NeoPixel DIN
#define NEOPIXEL_DATA_PIN 27
#define NEOPIXEL_COUNT 12
#define NEOPIXEL_MAX_BRIGHTNESS 160
#define NEOPIXEL_COLOR_ORDER_GRB 1

// NeoPixel effect timing.
// Higher period values make transitions slower and calmer.
#define NEOPIXEL_FRAME_INTERVAL_MS 50
#define RGB_BREATHING_PERIOD_MS 10000
#define RGB_SLOW_RAINBOW_PERIOD_MS 36000
#define RGB_WARM_GLOW_PERIOD_MS 14000
#define RGB_WATER_SHIMMER_PERIOD_MS 9000

// Keep green at 100% for NeoPixel color accuracy.
#define RGB_GREEN_CALIBRATION_PERCENT 100

// Legacy analog PWM pin config kept for fallback/testing if RGB_HARDWARE_TYPE is changed back.
#define RGB_RED_PIN 27
#define RGB_GREEN_PIN 14
#define RGB_BLUE_PIN 13
#define RGB_RED_CHANNEL 2
#define RGB_GREEN_CHANNEL 3
#define RGB_BLUE_CHANNEL 4
#define RGB_PWM_FREQUENCY 1000
#define RGB_PWM_RESOLUTION_BITS 8
#define RGB_OUTPUT_ACTIVE_HIGH 1
