#include "OfflineTimeline.h"

void OfflineTimeline::update(const FountainDailyTimeline &timeline, const DeviceClock &clock)
{
  if (!timeline.enabled)
  {
    if (lastActiveRangeIndex != -1)
    {
      Serial.println("OfflineTimeline inactive: daily timeline disabled.");
      lastActiveRangeIndex = -1;
    }
    return;
  }

  if (!clock.isTimeValid())
  {
    if (lastActiveRangeIndex != -1)
    {
      Serial.println("OfflineTimeline inactive: device time is not valid.");
      lastActiveRangeIndex = -1;
    }
    return;
  }

  int localMinute = clock.localMinutesOfDay();
  int activeIndex = -1;

  for (int i = 0; i < timeline.rangeCount; i++)
  {
    const FountainTimelineRange &range = timeline.ranges[i];

    if (isRangeActive(range, localMinute))
    {
      activeIndex = i;
      break;
    }
  }

  if (activeIndex == lastActiveRangeIndex)
  {
    return;
  }

  lastActiveRangeIndex = activeIndex;

  if (activeIndex < 0)
  {
    Serial.print("OfflineTimeline: no active range at local time ");
    Serial.println(clock.localTimeHHMM());
    return;
  }

  const FountainTimelineRange &activeRange = timeline.ranges[activeIndex];

  Serial.print("OfflineTimeline active range: ");
  Serial.print(activeRange.period.length() ? activeRange.period : "unknown");
  Serial.print(" scene: ");
  Serial.print(activeRange.sceneName.length() ? activeRange.sceneName : "missing scene");
  Serial.print(" local_time=");
  Serial.print(clock.localTimeHHMM());
  Serial.print(" range=");
  Serial.print(activeRange.startMinute);
  Serial.print("->");
  Serial.println(activeRange.endMinute);
}

bool OfflineTimeline::applyActiveRangeIfNeeded(
    const FountainDailyTimeline &timeline,
    const DeviceClock &clock,
    FountainOutputState &outputs,
    FountainReadings &readings,
    FountainOutputs &fountainOutputs)
{
  update(timeline, clock);

  if (lastActiveRangeIndex < 0 || lastActiveRangeIndex >= timeline.rangeCount)
  {
    return false;
  }

  if (lastOfflineAppliedRangeIndex == lastActiveRangeIndex)
  {
    return false;
  }

  const FountainTimelineRange &range = timeline.ranges[lastActiveRangeIndex];

  Serial.print("OfflineTimeline applying cached range: ");
  Serial.print(range.period.length() ? range.period : "unknown");
  Serial.print(" scene: ");
  Serial.println(range.sceneName.length() ? range.sceneName : "missing scene");

  bool applied = applyRangeOutputs(range, outputs, readings, fountainOutputs);

  if (applied)
  {
    lastOfflineAppliedRangeIndex = lastActiveRangeIndex;
  }

  return applied;
}

int OfflineTimeline::activeRangeIndex() const
{
  return lastActiveRangeIndex;
}

int OfflineTimeline::lastAppliedRangeIndex() const
{
  return lastOfflineAppliedRangeIndex;
}

bool OfflineTimeline::isRangeActive(const FountainTimelineRange &range, int localMinute) const
{
  if (!range.valid || range.startMinute < 0 || range.endMinute < 0 || localMinute < 0)
  {
    return false;
  }

  if (range.startMinute == range.endMinute)
  {
    // Avoid treating a zero-length range as active all day. If we ever need
    // all-day scenes, represent them explicitly later.
    return false;
  }

  if (range.startMinute < range.endMinute)
  {
    return localMinute >= range.startMinute && localMinute < range.endMinute;
  }

  // Cross-midnight range, e.g. 23:00 -> 06:00.
  return localMinute >= range.startMinute || localMinute < range.endMinute;
}

bool OfflineTimeline::applyRangeOutputs(
    const FountainTimelineRange &range,
    FountainOutputState &outputs,
    FountainReadings &readings,
    FountainOutputs &fountainOutputs)
{
  // Reuse the same product-safe output path as dashboard and scene commands.
  // This keeps water-low pump safety active during offline timeline execution.
  JsonDocument doc;
  JsonObject rangeOutputs = doc["outputs"].to<JsonObject>();

  JsonObject pump = rangeOutputs["pump"].to<JsonObject>();
  pump["enabled"] = range.outputs.pumpEnabled;
  pump["speed_percent"] = range.outputs.pumpSpeedPercent;

  JsonObject cob = rangeOutputs["cob_light"].to<JsonObject>();
  cob["enabled"] = range.outputs.cobEnabled;
  cob["brightness_percent"] = range.outputs.cobBrightnessPercent;

  JsonObject rgb = rangeOutputs["rgb_light"].to<JsonObject>();
  rgb["enabled"] = range.outputs.rgbEnabled;
  rgb["brightness_percent"] = range.outputs.rgbBrightnessPercent;
  rgb["color"] = range.outputs.rgbColor;
  rgb["effect"] = range.outputs.rgbEffect;

  bool allApplied = true;

  if (!fountainOutputs.applyOutput("pump", rangeOutputs["pump"].as<JsonObject>(), "offline_timeline", outputs, readings))
  {
    allApplied = false;
  }

  if (!fountainOutputs.applyOutput("cob_light", rangeOutputs["cob_light"].as<JsonObject>(), "offline_timeline", outputs, readings))
  {
    allApplied = false;
  }

  if (!fountainOutputs.applyOutput("rgb_light", rangeOutputs["rgb_light"].as<JsonObject>(), "offline_timeline", outputs, readings))
  {
    allApplied = false;
  }

  fountainOutputs.enforceWaterSafety(outputs, readings);
  return allApplied;
}
