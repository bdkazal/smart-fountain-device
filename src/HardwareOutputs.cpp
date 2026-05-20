#include "HardwareOutputs.h"

#if __has_include("HardwarePins.h")
#include "HardwarePins.h"
#else
#include "HardwarePins.example.h"
#endif

#ifndef PUMP_OUTPUT_ACTIVE_HIGH
#define PUMP_OUTPUT_ACTIVE_HIGH 1
#endif

void HardwareOutputs::begin()
{
  hardwareEnabled = SMART_FOUNTAIN_HARDWARE_ENABLED == 1;

  if (!hardwareEnabled)
  {
    Serial.println("HardwareOutputs disabled: running software-only output state.");
    return;
  }

  if (PUMP_OUTPUT_PIN >= 0)
  {
    pinMode(PUMP_OUTPUT_PIN, OUTPUT);
    digitalWrite(PUMP_OUTPUT_PIN, PUMP_OUTPUT_ACTIVE_HIGH == 1 ? LOW : HIGH);
    Serial.print("HardwareOutputs pump pin ready: GPIO ");
    Serial.print(PUMP_OUTPUT_PIN);
    Serial.print(" active_");
    Serial.println(PUMP_OUTPUT_ACTIVE_HIGH == 1 ? "HIGH" : "LOW");
  }
  else
  {
    Serial.println("HardwareOutputs pump pin not configured.");
  }

  if (COB_PWM_PIN >= 0)
  {
    ledcSetup(COB_PWM_CHANNEL, COB_PWM_FREQUENCY, COB_PWM_RESOLUTION_BITS);
    ledcAttachPin(COB_PWM_PIN, COB_PWM_CHANNEL);
    ledcWrite(COB_PWM_CHANNEL, 0);
    Serial.print("HardwareOutputs COB PWM ready: GPIO ");
    Serial.println(COB_PWM_PIN);
  }
  else
  {
    Serial.println("HardwareOutputs COB PWM pin not configured.");
  }

  Serial.println("HardwareOutputs RGB driver is placeholder/TBD.");
}

void HardwareOutputs::apply(const FountainOutputState &outputs)
{
  if (!hardwareEnabled)
  {
    return;
  }

  applyPump(outputs);
  applyCob(outputs);
  applyRgb(outputs);
}

bool HardwareOutputs::isEnabled() const
{
  return hardwareEnabled;
}

void HardwareOutputs::applyPump(const FountainOutputState &outputs)
{
  if (PUMP_OUTPUT_PIN < 0)
  {
    return;
  }

  bool gpioOn = PUMP_OUTPUT_ACTIVE_HIGH == 1;
  int onLevel = gpioOn ? HIGH : LOW;
  int offLevel = gpioOn ? LOW : HIGH;

  digitalWrite(PUMP_OUTPUT_PIN, outputs.pumpEnabled ? onLevel : offLevel);
}

void HardwareOutputs::applyCob(const FountainOutputState &outputs)
{
  if (COB_PWM_PIN < 0)
  {
    return;
  }

  int dutyMax = (1 << COB_PWM_RESOLUTION_BITS) - 1;
  int duty = outputs.cobEnabled ? map(outputs.cobBrightnessPercent, 0, 100, 0, dutyMax) : 0;
  ledcWrite(COB_PWM_CHANNEL, constrain(duty, 0, dutyMax));
}

void HardwareOutputs::applyRgb(const FountainOutputState &outputs)
{
  // RGB hardware is intentionally not guessed yet. We need to know whether the
  // final module is addressable RGB, three-channel PWM RGB, or another driver.
  (void)outputs;
}
