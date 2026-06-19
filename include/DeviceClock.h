#pragma once

#include <Arduino.h>

class DeviceClock
{
public:
  // Syncs the device clock from Laravel's UTC timestamp and the device timezone
  // offset. Offline timeline must not run until this returns true at least once.
  bool syncFromServerTime(const String &serverTimeUtc, int timezoneOffsetMinutes);

  // Restores the device clock from an already-parsed UTC epoch, usually DS3231 RTC.
  bool syncFromUtcEpoch(unsigned long utcEpochSeconds, int timezoneOffsetMinutes, const char *sourceLabel);

  bool isTimeValid() const;

  // Returns local minutes since midnight: 0..1439.
  // Returns -1 when time is not valid.
  int localMinutesOfDay() const;

  // Returns HH:MM for logs/debug. Returns "--:--" when time is not valid.
  String localTimeHHMM() const;

  long timezoneOffsetMinutes() const;
  unsigned long utcEpochSeconds() const;
  String sourceName() const;

private:
  bool timeValid = false;
  unsigned long syncedAtMillis = 0;
  unsigned long syncedUtcEpochSeconds = 0;
  long tzOffsetMinutes = 0;
  String timeSource = "NONE";

  bool parseServerTimeUtc(const String &serverTimeUtc, unsigned long &epochSeconds) const;
  bool isReasonableEpoch(unsigned long epochSeconds) const;
  bool isLeapYear(int year) const;
  int daysBeforeMonth(int year, int month) const;
};
