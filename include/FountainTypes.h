#pragma once

#include <Arduino.h>

struct FountainOutputState
{
  bool pumpEnabled = false;
  int pumpSpeedPercent = 0;

  bool cobEnabled = false;
  int cobBrightnessPercent = 0;

  bool rgbEnabled = false;
  int rgbBrightnessPercent = 0;
  String rgbColor = "#000000";
  String rgbEffect = "solid";
};

struct FountainReadings
{
  bool waterLow = false;
  int waterLevelPercent = 50;
  int waterLevelRaw = 2048;
};
