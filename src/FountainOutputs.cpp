#include "FountainOutputs.h"

void FountainOutputs::enforceWaterSafety(FountainOutputState &outputs, const FountainReadings &readings)
{
  // This is the most important local product safety rule.
  // Laravel may request pump ON, but the firmware must always protect the pump
  // when the water-level reading says the fountain is dry/low.
  if (!readings.waterLow)
  {
    return;
  }

  if (outputs.pumpEnabled || outputs.pumpSpeedPercent != 0)
  {
    Serial.println("Water safety active: forcing pump OFF.");
  }

  outputs.pumpEnabled = false;
  outputs.pumpSpeedPercent = 0;
}

bool FountainOutputs::applyOutput(const char *outputKey, JsonObject state, const char *source, FountainOutputState &outputs, const FountainReadings &readings)
{
  String key = outputKey;

  if (key == "pump")
  {
    applyPumpState(state, source, outputs, readings);
    return true;
  }

  if (key == "cob_light")
  {
    applyCobState(state, source, outputs);
    return true;
  }

  if (key == "rgb_light")
  {
    applyRgbState(state, source, outputs);
    return true;
  }

  Serial.print("Unknown output key: ");
  Serial.println(key);
  return false;
}

void FountainOutputs::applyPumpState(JsonObject state, const char *source, FountainOutputState &outputs, const FountainReadings &readings)
{
  bool requestedEnabled = state["enabled"] | false;
  int requestedSpeed = constrain((int)(state["speed_percent"] | 0), 0, 100);

  if (!requestedEnabled)
  {
    requestedSpeed = 0;
  }

  if (readings.waterLow && requestedEnabled)
  {
    Serial.println("Pump ON ignored because water_low=true.");
    outputs.pumpEnabled = false;
    outputs.pumpSpeedPercent = 0;
    return;
  }

  outputs.pumpEnabled = requestedEnabled;
  outputs.pumpSpeedPercent = requestedSpeed;

  Serial.print("Pump state applied from ");
  Serial.print(source);
  Serial.print(": enabled=");
  Serial.print(outputs.pumpEnabled ? "true" : "false");
  Serial.print(" speed=");
  Serial.println(outputs.pumpSpeedPercent);
}

void FountainOutputs::applyCobState(JsonObject state, const char *source, FountainOutputState &outputs)
{
  outputs.cobEnabled = state["enabled"] | false;
  outputs.cobBrightnessPercent = constrain((int)(state["brightness_percent"] | 0), 0, 100);

  if (!outputs.cobEnabled)
  {
    outputs.cobBrightnessPercent = 0;
  }

  Serial.print("COB state applied from ");
  Serial.print(source);
  Serial.print(": enabled=");
  Serial.print(outputs.cobEnabled ? "true" : "false");
  Serial.print(" brightness=");
  Serial.println(outputs.cobBrightnessPercent);
}

void FountainOutputs::applyRgbState(JsonObject state, const char *source, FountainOutputState &outputs)
{
  outputs.rgbEnabled = state["enabled"] | false;
  outputs.rgbBrightnessPercent = constrain((int)(state["brightness_percent"] | 0), 0, 100);
  outputs.rgbColor = String((const char *)(state["color"] | outputs.rgbColor.c_str()));
  outputs.rgbEffect = String((const char *)(state["effect"] | outputs.rgbEffect.c_str()));

  if (!outputs.rgbEnabled)
  {
    outputs.rgbBrightnessPercent = 0;
  }

  Serial.print("RGB state applied from ");
  Serial.print(source);
  Serial.print(": enabled=");
  Serial.print(outputs.rgbEnabled ? "true" : "false");
  Serial.print(" brightness=");
  Serial.print(outputs.rgbBrightnessPercent);
  Serial.print(" color=");
  Serial.print(outputs.rgbColor);
  Serial.print(" effect=");
  Serial.println(outputs.rgbEffect);
}
