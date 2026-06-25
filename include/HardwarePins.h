#pragma once

// Hardware pin configuration for the Biztola Smart Fountain on ESP32 DevKit V1 / ESP32-WROOM-32.
//
// Current hardware test scope:
// - Pump controlled through LR7843 MOSFET module
// - Normal switch / float switch connected to GPIO and GND for water safety
// - COB controlled as ON/OFF through a MOSFET/driver signal
// - 12 LED WS2812B / NeoPixel ring or strip controlled through one data pin
// - Local pump/COB buttons connected to GPIO and GND
// - Network/server and water-safety indicator LEDs through resistors
// - HW-111 / DS3231 RTC on I2C
//
// IMPORTANT:
// - ESP32 GPIO pins must never receive 12V or 5V.
// - Pump/COB/NeoPixel power must go through the external supply and driver/load path.
// - ESP32 GND, power-supply GND, MOSFET GND, buck GND, NeoPixel GND, and RTC GND must be common.
// - Keep the pump in water before ON testing.
// - Do not connect indicator LEDs directly without a current-limiting resistor.
// - Local buttons connect GPIO to GND only; never connect 5V/12V to button GPIO pins.
// - Power the HW-111 / DS3231 RTC from 3V3 for ESP32-safe I2C pull-ups.
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

// Status indicator LEDs.
// Wiring:
// GPIO23 -> resistor -> network/server LED -> GND
// GPIO13 -> resistor -> water safety LED -> GND
// ACTIVE_HIGH means GPIO HIGH turns the indicator LED ON.
#define NET_STATUS_LED_PIN 23
#define NET_STATUS_LED_ACTIVE_HIGH 1
#define WATER_SAFETY_LED_PIN 13
#define WATER_SAFETY_LED_ACTIVE_HIGH 1
#define NET_STATUS_FAST_BLINK_MS 250
#define NET_STATUS_SLOW_BLINK_MS 1000

// Local physical control buttons.
// Wiring:
// GPIO18 -> pump button -> GND
// GPIO19 -> COB button  -> GND
// Internal pull-up keeps each pin HIGH when the button is not pressed.
#define LOCAL_PUMP_BUTTON_PIN 18
#define LOCAL_COB_BUTTON_PIN 19
#define LOCAL_BUTTONS_ACTIVE_LOW 1
#define LOCAL_BUTTON_DEBOUNCE_MS 50

// HW-111 / DS3231 RTC I2C.
// Wiring:
// RTC VCC -> ESP32 3V3
// RTC GND -> ESP32 GND
// RTC SDA -> GPIO21
// RTC SCL -> GPIO22
#define RTC_I2C_SDA_PIN 21
#define RTC_I2C_SCL_PIN 22
#define RTC_UPDATE_DRIFT_THRESHOLD_SECONDS 5

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

// WS2812B / NeoPixel ring or strip.
// Laravel still sends this as logical rgb_light: enabled, brightness_percent, color, effect.
// Firmware renders that logical state to a 12 LED WS2812B/NeoPixel output.
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
