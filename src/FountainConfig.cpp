#include "FountainConfig.h"

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
    bool enabled = sourceState["enabled"] | false;
    int speed = constrain((int)(sourceState["speed_percent"] | 0), 0, 100);
    targetState["enabled"] = enabled;
    targetState["speed_percent"] = enabled ? speed : 0;
    return;
  }

  if (strcmp(key, "cob_light") == 0)
  {
    bool enabled = sourceState["enabled"] | false;
    int brightness = constrain((int)(sourceState["brightness_percent"] | 0), 0, 100);
    targetState["enabled"] = enabled;
    targetState["brightness_percent"] = enabled ? brightness : 0;
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

  JsonObject pumpOutput = configOutputs["pump"].as<JsonObject>();
  if (!pumpOutput.isNull())
  {
    JsonObject state = outputStateObject(pumpOutput);
    outputs.pumpEnabled = state["enabled"] | false;
    outputs.pumpSpeedPercent = constrain((int)(state["speed_percent"] | 0), 0, 100);

    if (!outputs.pumpEnabled)
    {
      outputs.pumpSpeedPercent = 0;
    }
  }

  JsonObject cobOutput = configOutputs["cob_light"].as<JsonObject>();
  if (!cobOutput.isNull())
  {
    JsonObject state = outputStateObject(cobOutput);
    outputs.cobEnabled = state["enabled"] | false;
    outputs.cobBrightnessPercent = constrain((int)(state["brightness_percent"] | 0), 0, 100);

    if (!outputs.cobEnabled)
    {
      outputs.cobBrightnessPercent = 0;
    }
  }

  JsonObject rgbOutput = configOutputs["rgb_light"].as<JsonObject>();
  if (!rgbOutput.isNull())
  {
    JsonObject state = outputStateObject(rgbOutput);
    outputs.rgbEnabled = state["enabled"] | false;
    outputs.rgbBrightnessPercent = constrain((int)(state["brightness_percent"] | 0), 0, 100);
    outputs.rgbColor = String((const char *)(state["color"] | outputs.rgbColor.c_str()));
    outputs.rgbEffect = String((const char *)(state["effect"] | outputs.rgbEffect.c_str()));

    if (!outputs.rgbEnabled)
    {
      outputs.rgbBrightnessPercent = 0;
    }
  }

  Serial.println("Initial output state loaded from Laravel config:");
  Serial.print(" - pump enabled=");
  Serial.print(outputs.pumpEnabled ? "true" : "false");
  Serial.print(" speed=");
  Serial.println(outputs.pumpSpeedPercent);

  Serial.print(" - cob_light enabled=");
  Serial.print(outputs.cobEnabled ? "true" : "false");
  Serial.print(" brightness=");
  Serial.println(outputs.cobBrightnessPercent);

  Serial.print(" - rgb_light enabled=");
  Serial.print(outputs.rgbEnabled ? "true" : "false");
  Serial.print(" brightness=");
  Serial.print(outputs.rgbBrightnessPercent);
  Serial.print(" color=");
  Serial.print(outputs.rgbColor);
  Serial.print(" effect=");
  Serial.println(outputs.rgbEffect);
}

bool FountainConfig::loadFromConfigObjectJson(const String &configJson, FountainOutputState &outputs)
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
  loadInitialOutputs(config, outputs);
  printDailyTimeline(config["daily_timeline"].as<JsonObject>());

  return true;
}

String FountainConfig::buildCompactCacheJson(JsonObject config)
{
  // This is intentionally not the full Laravel config. It is the smallest useful
  // firmware cache for boot restore and upcoming offline daily timeline support.
  JsonDocument compact;

  compact["cache_version"] = 1;
  compact["device_type"] = config["device_type"] | "smart_fountain";
  compact["timezone_offset_minutes"] = config["timezone_offset_minutes"] | 0;

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

void FountainConfig::printDailyTimeline(JsonObject dailyTimeline)
{
  // The Smart Fountain timeline is intentionally daily-only for V1. The backend
  // creates online scene commands, and firmware will later reuse this cached
  // timeline when internet/Laravel is unavailable.
  lastTimelineRepeat = dailyTimeline["repeat"] | "";

  Serial.print("Daily timeline enabled: ");
  Serial.println((bool)(dailyTimeline["enabled"] | false) ? "yes" : "no");

  Serial.print("Daily timeline repeat: ");
  Serial.println(lastTimelineRepeat.length() ? lastTimelineRepeat : "missing");

  JsonArray ranges = dailyTimeline["ranges"].as<JsonArray>();

  Serial.print("Timeline range count: ");
  Serial.println(ranges.size());

  for (JsonObject range : ranges)
  {
    const char *period = range["period"] | "unknown";
    const char *startTime = range["start_time"] | "--:--";
    const char *endTime = range["end_time"] | "--:--";
    const char *sceneName = range["scene_name"] | "missing scene";

    Serial.print(" - ");
    Serial.print(period);
    Serial.print(" ");
    Serial.print(startTime);
    Serial.print(" -> ");
    Serial.print(endTime);
    Serial.print(" scene: ");
    Serial.println(sceneName);
  }
}

String FountainConfig::timelineRepeat() const
{
  return lastTimelineRepeat;
}
