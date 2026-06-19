#pragma once

#include <Arduino.h>
#include "FountainTypes.h"

class CobOutputDriver
{
public:
  void begin();
  void apply(const FountainOutputState &outputs);

private:
  bool hasLoggedCobLevel = false;
  bool lastCobOn = false;
};
