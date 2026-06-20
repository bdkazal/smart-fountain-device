#pragma once

#include <Arduino.h>
#include "DeviceClock.h"
#include "FountainOutputs.h"
#include "FountainTypes.h"

struct DailyTimelineResult
{
  bool applied = false;
  const char *stateSource = "device_state";
  const char *reason = "";
};

class DailyTimelineRuntime
{
public:
  DailyTimelineResult update(
      const FountainDailyTimeline &timeline,
      const DeviceClock &clock,
      bool cloudAvailable,
      FountainOutputState &outputs,
      FountainReadings &readings,
      FountainOutputs &fountainOutputs);

  void markCurrentRangeSatisfied(
      const FountainDailyTimeline &timeline,
      const DeviceClock &clock,
      const char *reason = nullptr);

private:
  int lastActiveRangeIndex = -2;
  int lastAppliedRangeIndex = -2;

  int findActiveRangeIndex(const FountainDailyTimeline &timeline, int localMinute) const;
  bool isRangeActive(const FountainTimelineRange &range, int localMinute) const;

  bool applyRangeOutputs(
      const FountainTimelineRange &range,
      FountainOutputState &outputs,
      FountainReadings &readings,
      FountainOutputs &fountainOutputs);
};
