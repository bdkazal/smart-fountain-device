#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "HttpDeviceApi.h"
#include "FountainConfig.h"
#include "FountainTypes.h"

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

  bool fetchConfig(
    HttpDeviceApi &httpDeviceApi,
    JsonDocument &doc,
    JsonObject &config,
    String &response,
    int &statusCode,
    String &serverTimeUtc,
    String &deviceType,
    int &timezoneOffsetMinutes,
    bool &parseOk
  );

  bool loadCachedConfig(
    FountainConfig &fountainConfig,
    FountainOutputState &outputs,
    FountainDailyTimeline &dailyTimeline,
    String &cachedConfigJson,
    bool &cacheExists
  );
};
