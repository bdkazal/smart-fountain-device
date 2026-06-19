#include "HardwareOutputs.h"

#if __has_include("HardwarePins.h")
#include "HardwarePins.h"
#else
#include "HardwarePins.example.h"
#endif

#ifndef SMART_FOUNTAIN_HARDWARE_ENABLED
#define SMART_FOUNTAIN_HARDWARE_ENABLED 0
#endif

void HardwareOutputs::begin()
{
  hardwareEnabled = SMART_FOUNTAIN_HARDWARE_ENABLED == 1;

  if (!hardwareEnabled)
  {
    Serial.println("HardwareOutputs disabled: running software-only output state.");
    return;
  }

  pumpDriver.begin();
  cobDriver.begin();
  rgbDriver.begin();
}

void HardwareOutputs::apply(const FountainOutputState &outputs)
{
  if (!hardwareEnabled)
  {
    return;
  }

  pumpDriver.apply(outputs);
  cobDriver.apply(outputs);
  rgbDriver.apply(outputs);
}

bool HardwareOutputs::isEnabled() const
{
  return hardwareEnabled;
}
