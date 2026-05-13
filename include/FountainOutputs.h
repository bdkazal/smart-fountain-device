#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "FountainTypes.h"

class FountainOutputs
{
public:
  // Safety is part of output control, not cloud control. The pump must be forced
  // OFF locally whenever water is low, even if Laravel requested it ON earlier.
  void enforceWaterSafety(FountainOutputState &outputs, const FountainReadings &readings);

  // Applies one output state from dashboard command, scene command, or cached
  // offline timeline. Returns false only when the output key is unknown.
  bool applyOutput(const char *outputKey, JsonObject state, const char *source, FountainOutputState &outputs, const FountainReadings &readings);

private:
  void applyPumpState(JsonObject state, const char *source, FountainOutputState &outputs, const FountainReadings &readings);
  void applyCobState(JsonObject state, const char *source, FountainOutputState &outputs);
  void applyRgbState(JsonObject state, const char *source, FountainOutputState &outputs);
};
