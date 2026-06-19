#include "DeviceClock.h"

bool DeviceClock::syncFromServerTime(const String &serverTimeUtc, int timezoneOffsetMinutesValue)
{
  unsigned long parsedEpochSeconds = 0;

  if (!parseServerTimeUtc(serverTimeUtc, parsedEpochSeconds))
  {
    Serial.print("DeviceClock sync failed. Invalid server_time_utc: ");
    Serial.println(serverTimeUtc.length() ? serverTimeUtc : "missing");
    return false;
  }

  return syncFromUtcEpoch(parsedEpochSeconds, timezoneOffsetMinutesValue, "Laravel UTC");
}

bool DeviceClock::syncFromUtcEpoch(unsigned long utcEpochSecondsValue, int timezoneOffsetMinutesValue, const char *sourceLabel)
{
  if (!isReasonableEpoch(utcEpochSecondsValue))
  {
    Serial.print("DeviceClock sync failed. Unreasonable UTC epoch: ");
    Serial.println(utcEpochSecondsValue);
    return false;
  }

  syncedUtcEpochSeconds = utcEpochSecondsValue;
  syncedAtMillis = millis();
  tzOffsetMinutes = timezoneOffsetMinutesValue;
  timeSource = sourceLabel == nullptr ? "UTC" : String(sourceLabel);
  timeValid = true;

  Serial.print("DeviceClock synced from ");
  Serial.print(timeSource);
  Serial.print(". Local time: ");
  Serial.print(localTimeHHMM());
  Serial.print(" timezone_offset_minutes=");
  Serial.println(tzOffsetMinutes);

  return true;
}

bool DeviceClock::isTimeValid() const
{
  return timeValid;
}

int DeviceClock::localMinutesOfDay() const
{
  if (!timeValid)
  {
    return -1;
  }

  unsigned long elapsedSeconds = (millis() - syncedAtMillis) / 1000UL;
  unsigned long currentUtcSeconds = syncedUtcEpochSeconds + elapsedSeconds;
  long localDaySeconds = (long)(currentUtcSeconds % 86400UL) + (tzOffsetMinutes * 60L);

  localDaySeconds %= 86400L;

  if (localDaySeconds < 0)
  {
    localDaySeconds += 86400L;
  }

  return (int)(localDaySeconds / 60L);
}

String DeviceClock::localTimeHHMM() const
{
  int minutes = localMinutesOfDay();

  if (minutes < 0)
  {
    return "--:--";
  }

  int hour = minutes / 60;
  int minute = minutes % 60;

  char buffer[6];
  snprintf(buffer, sizeof(buffer), "%02d:%02d", hour, minute);

  return String(buffer);
}

long DeviceClock::timezoneOffsetMinutes() const
{
  return tzOffsetMinutes;
}

unsigned long DeviceClock::utcEpochSeconds() const
{
  if (!timeValid)
  {
    return 0;
  }

  unsigned long elapsedSeconds = (millis() - syncedAtMillis) / 1000UL;
  return syncedUtcEpochSeconds + elapsedSeconds;
}

String DeviceClock::sourceName() const
{
  return timeSource;
}

bool DeviceClock::parseServerTimeUtc(const String &serverTimeUtc, unsigned long &epochSeconds) const
{
  // Expected Laravel format: 2026-05-13T17:53:23+00:00
  // For V1, timezone in the string must be UTC (+00:00 or Z). Device-local
  // conversion is handled separately through timezone_offset_minutes.
  if (serverTimeUtc.length() < 19)
  {
    return false;
  }

  int year = serverTimeUtc.substring(0, 4).toInt();
  int month = serverTimeUtc.substring(5, 7).toInt();
  int day = serverTimeUtc.substring(8, 10).toInt();
  int hour = serverTimeUtc.substring(11, 13).toInt();
  int minute = serverTimeUtc.substring(14, 16).toInt();
  int second = serverTimeUtc.substring(17, 19).toInt();

  if (year < 2025 || year > 2099 || month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59)
  {
    return false;
  }

  unsigned long days = 0;

  for (int y = 1970; y < year; y++)
  {
    days += isLeapYear(y) ? 366UL : 365UL;
  }

  days += daysBeforeMonth(year, month);
  days += day - 1;

  epochSeconds = (days * 86400UL) + ((unsigned long)hour * 3600UL) + ((unsigned long)minute * 60UL) + (unsigned long)second;

  return isReasonableEpoch(epochSeconds);
}

bool DeviceClock::isReasonableEpoch(unsigned long epochSeconds) const
{
  // 2025-01-01 00:00:00 UTC through 2099-12-31-ish.
  return epochSeconds >= 1735689600UL && epochSeconds <= 4102444800UL;
}

bool DeviceClock::isLeapYear(int year) const
{
  if (year % 400 == 0)
  {
    return true;
  }

  if (year % 100 == 0)
  {
    return false;
  }

  return year % 4 == 0;
}

int DeviceClock::daysBeforeMonth(int year, int month) const
{
  static const int normalYearDaysBeforeMonth[] = {
      0,
      31,
      59,
      90,
      120,
      151,
      181,
      212,
      243,
      273,
      304,
      334,
  };

  int days = normalYearDaysBeforeMonth[month - 1];

  if (month > 2 && isLeapYear(year))
  {
    days += 1;
  }

  return days;
}
