#pragma once

#include <Arduino.h>
#include "FountainTypes.h"

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
  bool hasLoggedPumpLevel = false;
  bool lastPumpOn = false;
  bool lastPumpAssistActive = false;
  int lastPumpSpeedPercent = -1;
  int lastPumpDuty = -1;
  bool pumpStartupBoostActive = false;
  unsigned long pumpStartupBoostUntil = 0;
  unsigned long lastPumpAssistKickAt = 0;
  bool hasLoggedCobLevel = false;
  bool lastCobOn = false;
  bool hasLoggedRgbLevel = false;
  bool lastRgbOn = false;
  int lastRgbBrightnessPercent = -1;
  String lastRgbColor = "";
  String lastRgbEffect = "";
  int lastRgbRedDuty = -1;
  int lastRgbGreenDuty = -1;
  int lastRgbBlueDuty = -1;

  void applyPump(const FountainOutputState &outputs);
  void applyCob(const FountainOutputState &outputs);
  void applyRgb(const FountainOutputState &outputs);
  int rgbDutyFromChannel(int channelValue, int brightnessPercent) const;
  void parseRgbColor(const String &hexColor, int &red, int &green, int &blue) const;
};
