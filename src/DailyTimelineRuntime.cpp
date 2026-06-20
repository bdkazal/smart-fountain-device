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

  if (!timeline.enabled || !clock.isTimeValid())
  {
    lastActiveRangeIndex = -1;
    return result;
  }

  int activeIndex = findActiveRangeIndex(timeline, clock.localMinutesOfDay());

  if (activeIndex != lastActiveRangeIndex)
  {
    lastActiveRangeIndex = activeIndex;

    if (activeIndex >= 0)
    {
      const FountainTimelineRange &range = timeline.ranges[activeIndex];
      Serial.print("DailyTimeline active range: ");
      Serial.print(range.period.length() ? range.period : "unknown");
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
  const char *source = cloudAvailable ? "device_timeline" : "offline_timeline";

  Serial.print("DailyTimeline applying range: ");
  Serial.println(range.period.length() ? range.period : "unknown");

  if (!applyRangeOutputs(range, source, outputs, readings, fountainOutputs))
  {
    return result;
  }

  lastAppliedRangeIndex = activeIndex;
  result.applied = true;
  result.stateSource = source;
  result.reason = cloudAvailable
    ? "device daily timeline changed output state"
    : "offline daily timeline changed output state";

  return result;
}

void DailyTimelineRuntime::markCurrentRangeSatisfied(const char *reason)
{
  if (lastActiveRangeIndex < 0 || lastAppliedRangeIndex == lastActiveRangeIndex)
  {
    return;
  }

  lastAppliedRangeIndex = lastActiveRangeIndex;
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
    const char *source,
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
