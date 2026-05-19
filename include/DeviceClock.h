#pragma once

#include <Arduino.h>

class DeviceClock
{
public:
  // Syncs the device clock from Laravel's UTC timestamp and the device timezone
  // offset. Offline timeline must not run until this returns true at least once.
  bool syncFromServerTime(const String &serverTimeUtc, int timezoneOffsetMinutes);

  bool isTimeValid() const;

  // Returns local minutes since midnight: 0..1439.
  // Returns -1 when time is not valid.
  int localMinutesOfDay() const;

  // Returns HH:MM for logs/debug. Returns "--:--" when time is not valid.
  String localTimeHHMM() const;

  long timezoneOffsetMinutes() const;

private:
  bool timeValid = false;
  unsigned long syncedAtMillis = 0;
  long syncedUtcEpochSeconds = 0;
  long tzOffsetMinutes = 0;

  bool parseServerTimeUtc(const String &serverTimeUtc, long &epochSeconds) const;
  bool isLeapYear(int year) const;
  int daysBeforeMonth(int year, int month) const;
};
