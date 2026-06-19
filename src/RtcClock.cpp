#include "RtcClock.h"

#include <RTClib.h>
#include <Wire.h>

#if __has_include("HardwarePins.h")
#include "HardwarePins.h"
#endif

#ifndef RTC_I2C_SDA_PIN
#define RTC_I2C_SDA_PIN 21
#endif

#ifndef RTC_I2C_SCL_PIN
#define RTC_I2C_SCL_PIN 22
#endif

#ifndef RTC_UPDATE_DRIFT_THRESHOLD_SECONDS
#define RTC_UPDATE_DRIFT_THRESHOLD_SECONDS 5
#endif

namespace
{
RTC_DS3231 rtc;

bool rtcAvailable = false;
bool rtcTimeValid = false;
String rtcStatusText = "RTC not initialized";

bool isReasonableRtcTime(const DateTime &dateTime)
{
  int year = dateTime.year();
  return year >= 2025 && year <= 2099;
}

void printDateTime(const DateTime &dateTime)
{
  char buffer[24];
  snprintf(
      buffer,
      sizeof(buffer),
      "%04d-%02d-%02d %02d:%02d:%02d",
      dateTime.year(),
      dateTime.month(),
      dateTime.day(),
      dateTime.hour(),
      dateTime.minute(),
      dateTime.second());
  Serial.println(buffer);
}
}

void beginRtcClock()
{
  Serial.println();
  Serial.println("Initializing DS3231/HW-111 RTC...");
  Serial.print("RTC SDA GPIO: ");
  Serial.println(RTC_I2C_SDA_PIN);
  Serial.print("RTC SCL GPIO: ");
  Serial.println(RTC_I2C_SCL_PIN);

  Wire.begin(RTC_I2C_SDA_PIN, RTC_I2C_SCL_PIN);

  rtcAvailable = rtc.begin();

  if (!rtcAvailable)
  {
    rtcTimeValid = false;
    rtcStatusText = "RTC not found";
    Serial.println("DS3231/HW-111 RTC not found on I2C bus.");
    return;
  }

  if (rtc.lostPower())
  {
    rtcTimeValid = false;
    rtcStatusText = "RTC lost power";
    Serial.println("DS3231/HW-111 RTC found but lost power. RTC time is not trusted yet.");
    return;
  }

  DateTime now = rtc.now();
  rtcTimeValid = isReasonableRtcTime(now);

  if (!rtcTimeValid)
  {
    rtcStatusText = "RTC invalid";
    Serial.print("DS3231/HW-111 RTC UTC time invalid: ");
    printDateTime(now);
    return;
  }

  rtcStatusText = "RTC ready";
  Serial.print("DS3231/HW-111 RTC ready UTC: ");
  printDateTime(now);
}

bool isRtcAvailable()
{
  return rtcAvailable;
}

bool isRtcTimeValid()
{
  return rtcAvailable && rtcTimeValid;
}

bool loadUtcEpochFromRtc(unsigned long &utcEpochSeconds)
{
  if (!isRtcTimeValid())
  {
    Serial.println("RTC time load skipped: DS3231/HW-111 is unavailable or invalid.");
    return false;
  }

  DateTime rtcNow = rtc.now();

  if (!isReasonableRtcTime(rtcNow))
  {
    rtcTimeValid = false;
    rtcStatusText = "RTC invalid";
    Serial.println("RTC time load failed: DS3231/HW-111 time became invalid.");
    return false;
  }

  utcEpochSeconds = rtcNow.unixtime();
  rtcStatusText = "RTC UTC time loaded";

  Serial.print("UTC time loaded from DS3231/HW-111: ");
  printDateTime(rtcNow);
  return true;
}

bool saveUtcEpochToRtc(unsigned long utcEpochSeconds)
{
  if (!rtcAvailable)
  {
    Serial.println("RTC update skipped: DS3231/HW-111 is not available.");
    return false;
  }

  DateTime systemUtcNow(utcEpochSeconds);

  if (!isReasonableRtcTime(systemUtcNow))
  {
    Serial.println("RTC update skipped: UTC time is not reasonable.");
    return false;
  }

  DateTime rtcNow = rtc.now();

  if (isReasonableRtcTime(rtcNow))
  {
    long driftSeconds = labs((long)systemUtcNow.unixtime() - (long)rtcNow.unixtime());

    Serial.print("DS3231/HW-111 UTC drift seconds: ");
    Serial.println(driftSeconds);

    if (driftSeconds <= RTC_UPDATE_DRIFT_THRESHOLD_SECONDS)
    {
      rtcTimeValid = true;
      rtcStatusText = "RTC already in sync";
      Serial.println("RTC update skipped: DS3231/HW-111 already within drift threshold.");
      return false;
    }
  }

  rtc.adjust(systemUtcNow);
  rtcTimeValid = true;
  rtcStatusText = "RTC synced from Laravel UTC time";

  Serial.print("DS3231/HW-111 updated from UTC time: ");
  printDateTime(systemUtcNow);
  return true;
}

String getRtcStatusText()
{
  return rtcStatusText;
}
