#include "HardwareOutputs.h"

#if __has_include("HardwarePins.h")
#include "HardwarePins.h"
#else
#include "HardwarePins.example.h"
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
    digitalWrite(PUMP_OUTPUT_PIN, LOW);
    Serial.print("HardwareOutputs pump pin ready: GPIO ");
    Serial.println(PUMP_OUTPUT_PIN);
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

  // LR7843 MOSFET module is treated as active-HIGH for V1 until hardware test
  // proves otherwise. If the module behaves differently, change this driver only.
  digitalWrite(PUMP_OUTPUT_PIN, outputs.pumpEnabled ? HIGH : LOW);
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
