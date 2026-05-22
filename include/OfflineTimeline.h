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

  // While Laravel is reachable, dashboard/API is the source of truth. Mark the
  // current active range as already satisfied so a temporary API outage does not
  // immediately overwrite the current cloud/dashboard state with the cached
  // range. Offline timeline should act on the next range boundary, not the
  // moment Laravel disconnects.
  void rememberCurrentRangeAsSatisfied(const char *reason = nullptr);

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
