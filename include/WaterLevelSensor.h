#pragma once

#include <Arduino.h>
#include "FountainTypes.h"

class WaterLevelSensor
{
public:
  void begin();
  void update(FountainReadings &readings);

private:
  bool switchConfigured = false;
  bool lastWaterLow = false;
  bool hasLoggedInitialState = false;
  bool debouncedWaterLow = false;
  bool lastRawSwitchWaterLow = false;
  unsigned long lastRawSwitchChangedAt = 0;

  bool readSwitchWaterLow() const;
  bool debouncedSwitchWaterLow();
  void applyWaterLowState(FountainReadings &readings, bool waterLow, const char *source);
};
