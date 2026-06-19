#pragma once

#include <Arduino.h>
#include "FountainTypes.h"
#include "PumpOutputDriver.h"
#include "CobOutputDriver.h"
#include "RgbOutputDriver.h"

class HardwareOutputs
{
public:
  // Initializes physical output pins only when hardware is explicitly enabled.
  // Until then this module logs software state only and avoids accidental GPIO use.
  void begin();

  // Writes the current logical output state to physical hardware if enabled.
  // This should be called after FountainOutputs has applied product rules and
  // water-low pump safety.
  void apply(const FountainOutputState &outputs);

  bool isEnabled() const;

private:
  bool hardwareEnabled = false;
  PumpOutputDriver pumpDriver;
  CobOutputDriver cobDriver;
  RgbOutputDriver rgbDriver;
};
