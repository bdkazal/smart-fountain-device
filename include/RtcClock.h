#pragma once

#include <Arduino.h>

void beginRtcClock();

bool isRtcAvailable();
bool isRtcTimeValid();

// Loads DS3231 UTC time as Unix epoch seconds when the RTC has valid time.
bool loadUtcEpochFromRtc(unsigned long &utcEpochSeconds);

// Saves UTC Unix epoch seconds to the DS3231 when drift is meaningful.
bool saveUtcEpochToRtc(unsigned long utcEpochSeconds);

String getRtcStatusText();
