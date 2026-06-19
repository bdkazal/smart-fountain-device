#pragma once

#include <Arduino.h>
#include "FountainTypes.h"

class RgbOutputDriver
{
public:
  void begin();
  void apply(const FountainOutputState &outputs);

private:
  bool hasLoggedRgbLevel = false;
  bool lastRgbOn = false;
  int lastRgbBrightnessPercent = -1;
  String lastRgbColor = "";
  String lastRgbEffect = "";
  int lastRgbRedDuty = -1;
  int lastRgbGreenDuty = -1;
  int lastRgbBlueDuty = -1;

  bool isAnimatedEffect(const String &effect) const;
  unsigned long safeEffectPeriod(unsigned long configuredPeriod, unsigned long fallbackPeriod) const;
  void renderNeoPixels(int red, int green, int blue);
  int rgbDutyFromChannel(int channelValue, int brightnessPercent) const;
  void parseRgbColor(const String &hexColor, int &red, int &green, int &blue) const;
};
