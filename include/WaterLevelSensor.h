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

  bool readSwitchWaterLow() const;
  void applyWaterLowState(FountainReadings &readings, bool waterLow, const char *source);
};
