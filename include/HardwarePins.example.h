#pragma once

// Copy this file to include/HardwarePins.h when real hardware is ready.
// Do not commit include/HardwarePins.h if it contains prototype-specific wiring.
//
// V1 hardware is intentionally guarded. Outputs stay software-only until
// SMART_FOUNTAIN_HARDWARE_ENABLED is set to 1 and safe pins are configured.

#define SMART_FOUNTAIN_HARDWARE_ENABLED 0

// Pump output through LR7843 MOSFET module.
// TBD: choose final ESP32-C3 GPIO after wiring review.
#define PUMP_OUTPUT_PIN -1

// COB light PWM output.
// TBD: choose final ESP32-C3 GPIO and driver circuit.
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
