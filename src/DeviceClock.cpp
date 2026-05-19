#include "DeviceClock.h"

bool DeviceClock::syncFromServerTime(const String &serverTimeUtc, int timezoneOffsetMinutesValue)
{
  long parsedEpochSeconds = 0;

  if (!parseServerTimeUtc(serverTimeUtc, parsedEpochSeconds))
  {
    Serial.print("DeviceClock sync failed. Invalid server_time_utc: ");
    Serial.println(serverTimeUtc.length() ? serverTimeUtc : "missing");
    return false;
  }

  syncedUtcEpochSeconds = parsedEpochSeconds;
  syncedAtMillis = millis();
  tzOffsetMinutes = timezoneOffsetMinutesValue;
  timeValid = true;

  Serial.print("DeviceClock synced. Local time: ");
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
  long currentUtcSeconds = syncedUtcEpochSeconds + (long)elapsedSeconds;
  long localSeconds = currentUtcSeconds + (tzOffsetMinutes * 60L);

  long secondsInDay = localSeconds % 86400L;

  if (secondsInDay < 0)
  {
    secondsInDay += 86400L;
  }

  return (int)(secondsInDay / 60L);
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

bool DeviceClock::parseServerTimeUtc(const String &serverTimeUtc, long &epochSeconds) const
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

  if (year < 2020 || month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59)
  {
    return false;
  }

  long days = 0;

  for (int y = 1970; y < year; y++)
  {
    days += isLeapYear(y) ? 366 : 365;
  }

  days += daysBeforeMonth(year, month);
  days += day - 1;

  epochSeconds = (days * 86400L) + (hour * 3600L) + (minute * 60L) + second;

  return true;
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
