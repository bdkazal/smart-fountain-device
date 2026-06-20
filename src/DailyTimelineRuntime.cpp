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
  return result;
}

void DailyTimelineRuntime::markCurrentRangeSatisfied(const char *reason)
{
}

int DailyTimelineRuntime::findActiveRangeIndex(const FountainDailyTimeline &timeline, int localMinute) const
{
  return -1;
}

bool DailyTimelineRuntime::isRangeActive(const FountainTimelineRange &range, int localMinute) const
{
  return false;
}

bool DailyTimelineRuntime::applyRangeOutputs(
    const FountainTimelineRange &range,
    const char *source,
    FountainOutputState &outputs,
    FountainReadings &readings,
    FountainOutputs &fountainOutputs)
{
  return false;
}
