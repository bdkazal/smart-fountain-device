#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

class ConfigRuntime
{
public:
  bool parseConfigResponse(
    const String &response,
    JsonDocument &doc,
    JsonObject &config,
    String &serverTimeUtc,
    String &deviceType,
    int &timezoneOffsetMinutes
  );
};
