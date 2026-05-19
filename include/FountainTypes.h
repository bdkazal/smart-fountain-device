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

const int MAX_DAILY_TIMELINE_RANGES = 6;

struct FountainTimelineRange
{
  bool valid = false;
  String period = "";
  String sceneName = "";
  int sceneId = 0;

  // Minutes since local midnight. Example: 06:30 => 390.
  int startMinute = -1;
  int endMinute = -1;

  // Output state to apply when this range is active. This comes from the compact
  // cached config, so offline timeline does not need to ask Laravel for scenes.
  FountainOutputState outputs;
};

struct FountainDailyTimeline
{
  bool enabled = false;
  String repeat = "daily";
  int rangeCount = 0;
  FountainTimelineRange ranges[MAX_DAILY_TIMELINE_RANGES];
};
