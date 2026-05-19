#pragma once

#include <Arduino.h>
#include "DeviceClock.h"
#include "FountainTypes.h"

class OfflineTimeline
{
public:
  // Detection-only mode for now. It finds the active cached daily range and logs
  // range changes, but does not apply outputs yet. This lets us prove schedule
  // matching before controlling hardware or changing device state.
  void update(const FountainDailyTimeline &timeline, const DeviceClock &clock);

  int activeRangeIndex() const;

private:
  int lastActiveRangeIndex = -2;

  bool isRangeActive(const FountainTimelineRange &range, int localMinute) const;
};
