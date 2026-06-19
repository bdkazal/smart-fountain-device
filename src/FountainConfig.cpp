#include "FountainConfig.h"

#include "DeviceClock.h"
#include "RtcClock.h"

extern DeviceClock deviceClock;

namespace
{
const int DEFAULT_TIMEZONE_OFFSET_MINUTES = 360;

void restoreClockFromRtcAfterCacheLoad(int timezoneOffsetMinutes)
{
  if (deviceClock.isTimeValid())
  {
    return;
  }

  beginRtcClock();

  unsigned long rtcUtcEpochSeconds = 0;

  if (!loadUtcEpochFromRtc(rtcUtcEpochSeconds))
  {
    return;
  }

  deviceClock.syncFromUtcEpoch(rtcUtcEpochSeconds, timezoneOffsetMinutes, "RTC");
}
}

JsonObject FountainConfig::outputStateObject(JsonObject outputConfig)
{
  JsonObject state = outputConfig["state"].as<JsonObject>();

  if (!state.isNull())
  {
    return state;
  }

  return outputConfig;
}

void FountainConfig::copyOutputState(JsonObject sourceOutputs, JsonObject targetOutputs, const char *key)
{
  // The compact cache stores only the state fields firmware needs to restore
  // output state and run an offline timeline. Dashboard-only metadata is ignored.
  JsonObject sourceOutput = sourceOutputs[key].as<JsonObject>();

  if (sourceOutput.isNull())
  {
    return;
  }

  JsonObject sourceState = outputStateObject(sourceOutput);
  JsonObject targetState = targetOutputs[key].to<JsonObject>();

  if (strcmp(key, "pump") == 0)
  {
    targetState["enabled"] = sourceState["enabled"] | false;
    return;
  }

  if (strcmp(key, "cob_light") == 0)
  {
    targetState["enabled"] = sourceState["enabled"] | false;
    return;
  }

  if (strcmp(key, "rgb_light") == 0)
  {
    bool enabled = sourceState["enabled"] | false;
    int brightness = constrain((int)(sourceState["brightness_percent"] | 0), 0, 100);
    targetState["enabled"] = enabled;
    targetState["brightness_percent"] = enabled ? brightness : 0;
    targetState["color"] = sourceState["color"] | "#000000";
    targetState["effect"] = sourceState["effect"] | "solid";
  }
}

void FountainConfig::loadOutputStateFromJson(JsonObject sourceOutputs, const char *key, FountainOutputState &targetOutputs)
{
  JsonObject sourceOutput = sourceOutputs[key].as<JsonObject>();

  if (sourceOutput.isNull())
  {
    return;
  }

  JsonObject state = outputStateObject(sourceOutput);

  if (strcmp(key, "pump") == 0)
  {
    targetOutputs.pumpEnabled = state["enabled"] | false;
    targetOutputs.pumpSpeedPercent = targetOutputs.pumpEnabled ? 100 : 0;
    return;
  }

  if (strcmp(key, "cob_light") == 0)
  {
    targetOutputs.cobEnabled = state["enabled"] | false;
    targetOutputs.cobBrightnessPercent = targetOutputs.cobEnabled ? 100 : 0;
    return;
  }

  if (strcmp(key, "rgb_light") == 0)
  {
    targetOutputs.rgbEnabled = state["enabled"] | false;
    targetOutputs.rgbBrightnessPercent = constrain((int)(state["brightness_percent"] | 0), 0, 100);
    targetOutputs.rgbColor = String((const char *)(state["color"] | targetOutputs.rgbColor.c_str()));
    targetOutputs.rgbEffect = String((const char *)(state["effect"] | targetOutputs.rgbEffect.c_str()));

    if (!targetOutputs.rgbEnabled)
    {
      targetOutputs.rgbBrightnessPercent = 0;
    }
  }
}

int FountainConfig::parseHHMMToMinutes(const char *hhmm) const
{
  if (hhmm == nullptr || strlen(hhmm) < 5)
  {
    return -1;
  }

  int hour = String(hhmm).substring(0, 2).toInt();
  int minute = String(hhmm).substring(3, 5).toInt();

  if (hour < 0 || hour > 23 || minute < 0 || minute > 59)
  {
    return -1;
  }

  return (hour * 60) + minute;
}

void FountainConfig::loadInitialOutputs(JsonObject config, FountainOutputState &outputs)
{
  // Boot should start from Laravel's latest known output state, then report the
  // actual device state back through /api/device/state. This avoids a reboot
  // accidentally overwriting the dashboard with all outputs OFF.
  JsonObject configOutputs = config["outputs"].as<JsonObject>();

  if (configOutputs.isNull())
  {
    Serial.println("No config.outputs found. Keeping safe default output state.");
    return;
  }

  loadOutputStateFromJson(configOutputs, "pump", outputs);
  loadOutputStateFromJson(configOutputs, "cob_light", outputs);
  loadOutputStateFromJson(configOutputs, "rgb_light", outputs);

  Serial.println("Initial output state loaded from Laravel config:");
  Serial.print(" - pump enabled=");
  Serial.print(outputs.pumpEnabled ? "true" : "false");
  Serial.println(" mode=on_off");

  Serial.print(" - cob_light enabled=");
  Serial.print(outputs.cobEnabled ? "true" : "false");
  Serial.println(" mode=on_off");

  Serial.print(" - rgb_light enabled=");
  Serial.print(outputs.rgbEnabled ? "true" : "false");
  Serial.print(" brightness=");
  Serial.print(outputs.rgbBrightnessPercent);
  Serial.print(" color=");
  Serial.print(outputs.rgbColor);
  Serial.print(" effect=");
  Serial.println(outputs.rgbEffect);
}

void FountainConfig::loadDailyTimeline(JsonObject dailyTimeline, FountainDailyTimeline &timeline)
{
  timeline = FountainDailyTimeline();
  timeline.enabled = dailyTimeline["enabled"] | false;
  timeline.repeat = String((const char *)(dailyTimeline["repeat"] | "daily"));
  lastTimelineRepeat = timeline.repeat;

  JsonArray ranges = dailyTimeline["ranges"].as<JsonArray>();

  for (JsonObject range : ranges)
  {
    if (timeline.rangeCount >= MAX_DAILY_TIMELINE_RANGES)
    {
      Serial.println("Daily timeline range limit reached; extra ranges ignored.");
      break;
    }

    FountainTimelineRange &targetRange = timeline.ranges[timeline.rangeCount];
    targetRange.valid = true;
    targetRange.period = String((const char *)(range["period"] | ""));
    targetRange.sceneName = String((const char *)(range["scene_name"] | ""));
    targetRange.sceneId = range["scene_id"] | 0;
    targetRange.startMinute = parseHHMMToMinutes(range["start_time"] | "");
    targetRange.endMinute = parseHHMMToMinutes(range["end_time"] | "");

    JsonObject rangeOutputs = range["outputs"].as<JsonObject>();
    loadOutputStateFromJson(rangeOutputs, "pump", targetRange.outputs);
    loadOutputStateFromJson(rangeOutputs, "cob_light", targetRange.outputs);
    loadOutputStateFromJson(rangeOutputs, "rgb_light", targetRange.outputs);

    timeline.rangeCount++;
  }
}

bool FountainConfig::loadFromConfigObjectJson(const String &configJson, FountainOutputState &outputs, FountainDailyTimeline &timeline)
{
  if (configJson.length() == 0)
  {
    Serial.println("No cached config JSON to parse.");
    return false;
  }

  // Heap-backed JsonDocument avoids large stack allocation on ESP32-C3.
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, configJson);

  if (error)
  {
    Serial.print("Failed to parse cached config JSON: ");
    Serial.println(error.c_str());
    return false;
  }

  JsonObject config = doc.as<JsonObject>();
  int timezoneOffsetMinutes = config["timezone_offset_minutes"] | DEFAULT_TIMEZONE_OFFSET_MINUTES;

  restoreClockFromRtcAfterCacheLoad(timezoneOffsetMinutes);
  loadInitialOutputs(config, outputs);
  loadDailyTimeline(config["daily_timeline"].as<JsonObject>(), timeline);
  printDailyTimeline(timeline);

  return true;
}

String FountainConfig::buildCompactCacheJson(JsonObject config)
{
  // This is intentionally not the full Laravel config. It is the smallest useful
  // firmware cache for boot restore and upcoming offline daily timeline support.
  JsonDocument compact;

  compact["cache_version"] = 2;
  compact["device_type"] = config["device_type"] | "smart_fountain";
  compact["timezone_offset_minutes"] = config["timezone_offset_minutes"] | DEFAULT_TIMEZONE_OFFSET_MINUTES;

  JsonObject sourceOutputs = config["outputs"].as<JsonObject>();
  JsonObject targetOutputs = compact["outputs"].to<JsonObject>();
  copyOutputState(sourceOutputs, targetOutputs, "pump");
  copyOutputState(sourceOutputs, targetOutputs, "cob_light");
  copyOutputState(sourceOutputs, targetOutputs, "rgb_light");

  JsonObject sourceTimeline = config["daily_timeline"].as<JsonObject>();
  JsonObject targetTimeline = compact["daily_timeline"].to<JsonObject>();
  targetTimeline["enabled"] = sourceTimeline["enabled"] | false;
  targetTimeline["repeat"] = sourceTimeline["repeat"] | "daily";

  JsonArray sourceRanges = sourceTimeline["ranges"].as<JsonArray>();
  JsonArray targetRanges = targetTimeline["ranges"].to<JsonArray>();

  for (JsonObject sourceRange : sourceRanges)
  {
    JsonObject targetRange = targetRanges.add<JsonObject>();
    targetRange["period"] = sourceRange["period"] | "";
    targetRange["start_time"] = sourceRange["start_time"] | "";
    targetRange["end_time"] = sourceRange["end_time"] | "";
    targetRange["scene_id"] = sourceRange["scene_id"] | 0;
    targetRange["scene_name"] = sourceRange["scene_name"] | "";

    JsonObject sourceRangeOutputs = sourceRange["outputs"].as<JsonObject>();
    JsonObject targetRangeOutputs = targetRange["outputs"].to<JsonObject>();
    copyOutputState(sourceRangeOutputs, targetRangeOutputs, "pump");
    copyOutputState(sourceRangeOutputs, targetRangeOutputs, "cob_light");
    copyOutputState(sourceRangeOutputs, targetRangeOutputs, "rgb_light");
  }

  String compactJson;
  serializeJson(compact, compactJson);

  return compactJson;
}

void FountainConfig::printDailyTimeline(const FountainDailyTimeline &timeline)
{
  // The Smart Fountain timeline is intentionally daily-only for V1. The backend
  // creates online scene commands, and firmware will later reuse this cached
  // timeline when internet/Laravel is unavailable. For offline use, each range
  // must include output states, not only scene names.
  Serial.print("Daily timeline enabled: ");
  Serial.println(timeline.enabled ? "yes" : "no");

  Serial.print("Daily timeline repeat: ");
  Serial.println(timeline.repeat.length() ? timeline.repeat : "missing");

  Serial.print("Timeline range count: ");
  Serial.println(timeline.rangeCount);

  for (int i = 0; i < timeline.rangeCount; i++)
  {
    const FountainTimelineRange &range = timeline.ranges[i];
    bool hasApplyReadyOutputs = range.valid && (range.startMinute >= 0) && (range.endMinute >= 0);

    Serial.print(" - ");
    Serial.print(range.period.length() ? range.period : "unknown");
    Serial.print(" ");
    Serial.print(range.startMinute);
    Serial.print(" -> ");
    Serial.print(range.endMinute);
    Serial.print(" scene: ");
    Serial.print(range.sceneName.length() ? range.sceneName : "missing scene");
    Serial.print(" outputs: ");
    Serial.println(hasApplyReadyOutputs ? "yes" : "missing");
  }
}

String FountainConfig::timelineRepeat() const
{
  return lastTimelineRepeat;
}
