#include "DailyTimelineRuntime.h"

DailyTimelineResult DailyTimelineRuntime::update(
    const FountainDailyTimeline &timeline,
    const DeviceClock &clock,
    bool cloudAvailable,
    FountainOutputState &outputs,
    FountainReadings &readings,
    FountainOutputs &fountainOutputs)
{
  DailyTimelineResult result;

  if (!timeline.enabled)
  {
    if (lastActiveRangeIndex != -1)
    {
      Serial.println("DailyTimeline inactive: daily timeline disabled.");
      lastActiveRangeIndex = -1;
    }

    return result;
  }

  if (!clock.isTimeValid())
  {
    if (lastActiveRangeIndex != -1)
    {
      Serial.println("DailyTimeline inactive: device time is not valid.");
      lastActiveRangeIndex = -1;
    }

    return result;
  }

  int activeIndex = findActiveRangeIndex(timeline, clock.localMinutesOfDay());

  if (activeIndex != lastActiveRangeIndex)
  {
    lastActiveRangeIndex = activeIndex;

    if (activeIndex < 0)
    {
      Serial.print("DailyTimeline: no active range at local time ");
      Serial.println(clock.localTimeHHMM());
    }
    else
    {
      const FountainTimelineRange &range = timeline.ranges[activeIndex];

      Serial.print("DailyTimeline active range: ");
      Serial.print(range.period.length() ? range.period : "unknown");
      Serial.print(" scene: ");
      Serial.print(range.sceneName.length() ? range.sceneName : "missing scene");
      Serial.print(" local_time=");
      Serial.println(clock.localTimeHHMM());
    }
  }

  if (activeIndex < 0 || activeIndex >= timeline.rangeCount)
  {
    return result;
  }

  if (lastAppliedRangeIndex == activeIndex)
  {
    return result;
  }

  const FountainTimelineRange &range = timeline.ranges[activeIndex];

  Serial.print("DailyTimeline applying range: ");
  Serial.print(range.period.length() ? range.period : "unknown");
  Serial.print(" scene: ");
  Serial.println(range.sceneName.length() ? range.sceneName : "missing scene");

  if (!applyRangeOutputs(range, outputs, readings, fountainOutputs))
  {
    return result;
  }

  lastAppliedRangeIndex = activeIndex;

  result.applied = true;
  result.stateSource = cloudAvailable ? "device_timeline" : "offline_timeline";
  result.reason = cloudAvailable
    ? "device daily timeline changed output state"
    : "offline daily timeline changed output state";

  return result;
}

void DailyTimelineRuntime::markCurrentRangeSatisfied(
    const FountainDailyTimeline &timeline,
    const DeviceClock &clock,
    const char *reason)
{
  if (!timeline.enabled || !clock.isTimeValid())
  {
    return;
  }

  int activeIndex = findActiveRangeIndex(timeline, clock.localMinutesOfDay());

  if (activeIndex < 0)
  {
    return;
  }

  lastActiveRangeIndex = activeIndex;
  lastAppliedRangeIndex = activeIndex;

  Serial.print("DailyTimeline current range marked satisfied");

  if (reason != nullptr && strlen(reason) > 0)
  {
    Serial.print(": ");
    Serial.print(reason);
  }

  Serial.println();
}

int DailyTimelineRuntime::findActiveRangeIndex(const FountainDailyTimeline &timeline, int localMinute) const
{
  for (int i = 0; i < timeline.rangeCount; i++)
  {
    if (isRangeActive(timeline.ranges[i], localMinute))
    {
      return i;
    }
  }

  return -1;
}

bool DailyTimelineRuntime::isRangeActive(const FountainTimelineRange &range, int localMinute) const
{
  if (!range.valid || range.startMinute < 0 || range.endMinute < 0 || localMinute < 0)
  {
    return false;
  }

  if (range.startMinute == range.endMinute)
  {
    return false;
  }

  if (range.startMinute < range.endMinute)
  {
    return localMinute >= range.startMinute && localMinute < range.endMinute;
  }

  return localMinute >= range.startMinute || localMinute < range.endMinute;
}

bool DailyTimelineRuntime::applyRangeOutputs(
    const FountainTimelineRange &range,
    FountainOutputState &outputs,
    FountainReadings &readings,
    FountainOutputs &fountainOutputs)
{
  outputs.pumpEnabled = range.outputs.pumpEnabled;
  outputs.pumpSpeedPercent = range.outputs.pumpSpeedPercent;

  outputs.cobEnabled = range.outputs.cobEnabled;
  outputs.cobBrightnessPercent = range.outputs.cobBrightnessPercent;

  outputs.rgbEnabled = range.outputs.rgbEnabled;
  outputs.rgbBrightnessPercent = range.outputs.rgbBrightnessPercent;
  outputs.rgbColor = range.outputs.rgbColor;
  outputs.rgbEffect = range.outputs.rgbEffect;

  fountainOutputs.enforceWaterSafety(outputs, readings);
  return true;
}
