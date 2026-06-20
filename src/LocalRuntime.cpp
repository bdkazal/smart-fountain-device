#include "LocalRuntime.h"

#include <Arduino.h>

bool LocalRuntime::processControls(
  LocalControls &localControls,
  FountainOutputState &outputs,
  FountainReadings &readings
)
{
  localControls.update();

  bool changed = false;

  if (localControls.consumePumpToggleRequest())
  {
    bool requestedEnabled = !outputs.pumpEnabled;

    if (requestedEnabled && readings.waterLow)
    {
      outputs.pumpEnabled = false;
      outputs.pumpSpeedPercent = 0;
      Serial.println("Local pump button ignored because water_low=true.");
    }
    else
    {
      outputs.pumpEnabled = requestedEnabled;
      outputs.pumpSpeedPercent = requestedEnabled ? 100 : 0;
      changed = true;

      Serial.print("Pump state applied from local_button: enabled=");
      Serial.print(outputs.pumpEnabled ? "true" : "false");
      Serial.println(" mode=on_off");
    }
  }

  if (localControls.consumeCobToggleRequest())
  {
    outputs.cobEnabled = !outputs.cobEnabled;
    outputs.cobBrightnessPercent = outputs.cobEnabled ? 100 : 0;
    changed = true;

    Serial.print("COB state applied from local_button: enabled=");
    Serial.print(outputs.cobEnabled ? "true" : "false");
    Serial.println(" mode=on_off");
  }

  return changed;
}
