#pragma once

#include <Arduino.h>
#include "DeviceClock.h"
#include "FountainOutputs.h"
#include "FountainTypes.h"

class OfflineTimeline
{
public:
  // Finds the active cached daily range and logs range changes. This is safe to
  // run while online because it does not change output state.
  void update(const FountainDailyTimeline &timeline, const DeviceClock &clock);

  // Applies the active cached range once when offline/server unavailable. This
  // must not be used while Laravel is reachable, otherwise cloud commands and
  // local schedule can fight each other.
  bool applyActiveRangeIfNeeded(
      const FountainDailyTimeline &timeline,
      const DeviceClock &clock,
      FountainOutputState &outputs,
      FountainReadings &readings,
      FountainOutputs &fountainOutputs);

  int activeRangeIndex() const;
  int lastAppliedRangeIndex() const;

private:
  int lastActiveRangeIndex = -2;
  int lastOfflineAppliedRangeIndex = -2;

  bool isRangeActive(const FountainTimelineRange &range, int localMinute) const;
  bool applyRangeOutputs(
      const FountainTimelineRange &range,
      FountainOutputState &outputs,
      FountainReadings &readings,
      FountainOutputs &fountainOutputs);
};
