#pragma once

// Prototype hardware pin configuration for the Biztola Smart Fountain.
//
// Current V1 hardware test scope:
// - Pump controlled through LR7843 MOSFET module
// - Normal switch / float switch connected to GPIO and GND for water safety
// - COB controlled as ON/OFF through a MOSFET/driver signal
// - Analog 5V 5050 RGB controlled through 3 AO3400 low-side MOSFET channels
//
// IMPORTANT:
// - ESP32 GPIO pins must never receive 12V or 5V.
// - Pump/COB/RGB power must go through the external supply and MOSFET/driver path.
// - ESP32 GND, power-supply GND, MOSFET GND, and buck GND must be common.
// - Keep the pump in water before ON testing.
// - Do not connect LEDs directly to ESP32 GPIO.

#define SMART_FOUNTAIN_HARDWARE_ENABLED 1

// Pump output through LR7843 MOSFET module.
// Wiring: GPIO5 -> LR7843 signal/input.
// ACTIVE_HIGH means GPIO HIGH turns the pump MOSFET/load ON.
#define PUMP_OUTPUT_PIN 5
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
// Wiring: GPIO0 -> switch -> GND.
// Internal pull-up keeps the pin HIGH when the switch is open.
// ACTIVE_LOW means closed switch reads LOW and becomes water_low=true.
#define WATER_LEVEL_SWITCH_PIN 0
#define WATER_LEVEL_SWITCH_ACTIVE_LOW 1
#define WATER_LEVEL_SWITCH_USE_PULLUP 1
#define WATER_LEVEL_SWITCH_DEBOUNCE_MS 150

// COB output through a MOSFET/driver signal.
// Wiring: GPIO6 -> COB MOSFET signal/input.
// ACTIVE_HIGH means GPIO HIGH turns the COB output ON.
// V1 is ON/OFF only. No brightness/PWM dimming yet.
#define COB_OUTPUT_PIN 6
#define COB_OUTPUT_ACTIVE_HIGH 1
#define COB_PWM_PIN -1
#define COB_PWM_CHANNEL 0
#define COB_PWM_FREQUENCY 5000
#define COB_PWM_RESOLUTION_BITS 8

// Analog 5V 5050 RGB through 3 AO3400 low-side MOSFET channels.
// Wiring:
// RGB +5V -> external 5V supply
// RGB R/G/B pads -> MOSFET drains
// MOSFET sources -> common GND
// ESP32 GPIO -> 100R -> MOSFET gate, gate -> 10k -> GND
#define RGB_HARDWARE_TYPE_NONE 0
#define RGB_HARDWARE_TYPE_ANALOG_PWM 1
#define RGB_HARDWARE_TYPE RGB_HARDWARE_TYPE_ANALOG_PWM
#define RGB_DATA_PIN -1
#define RGB_RED_PIN 2
#define RGB_GREEN_PIN 3
#define RGB_BLUE_PIN 4
#define RGB_RED_CHANNEL 2
#define RGB_GREEN_CHANNEL 3
#define RGB_BLUE_CHANNEL 4
#define RGB_PWM_FREQUENCY 1000
#define RGB_PWM_RESOLUTION_BITS 8
#define RGB_OUTPUT_ACTIVE_HIGH 1
