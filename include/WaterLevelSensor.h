#pragma once

#include <Arduino.h>
#include "FountainTypes.h"

class WaterLevelSensor
{
public:
  void begin();
  void update(FountainReadings &readings);
};
