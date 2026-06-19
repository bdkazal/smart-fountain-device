#pragma once

#include <Arduino.h>
#include "FountainTypes.h"

class PumpOutputDriver
{
public:
  void begin();
  void apply(const FountainOutputState &outputs);

private:
  bool hasLoggedPumpLevel = false;
  bool lastPumpOn = false;
  bool lastPumpAssistActive = false;
  int lastPumpSpeedPercent = -1;
  int lastPumpDuty = -1;
  bool pumpStartupBoostActive = false;
  unsigned long pumpStartupBoostUntil = 0;
  unsigned long lastPumpAssistKickAt = 0;
};
