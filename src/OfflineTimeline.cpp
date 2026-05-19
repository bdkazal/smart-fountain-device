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

int OfflineTimeline::activeRangeIndex() const
{
  return lastActiveRangeIndex;
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
