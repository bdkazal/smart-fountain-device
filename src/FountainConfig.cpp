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
