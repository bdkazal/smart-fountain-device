#include "ConfigRuntime.h"
#include "ConfigCache.h"

bool ConfigRuntime::parseConfigResponse(
  const String &response,
  JsonDocument &doc,
  JsonObject &config,
  String &serverTimeUtc,
  String &deviceType,
  int &timezoneOffsetMinutes
)
{
  DeserializationError error = deserializeJson(doc, response);

  if (error)
  {
    Serial.print("Config JSON parse failed: ");
    Serial.println(error.c_str());
    return false;
  }

  serverTimeUtc = doc["server_time_utc"] | "";

  config = doc["config"].as<JsonObject>();
  deviceType = config["device_type"] | "";
  timezoneOffsetMinutes = config["timezone_offset_minutes"] | 0;

  return true;
}

bool ConfigRuntime::fetchConfig(
  HttpDeviceApi &httpDeviceApi,
  JsonDocument &doc,
  JsonObject &config,
  String &response,
  int &statusCode,
  String &serverTimeUtc,
  String &deviceType,
  int &timezoneOffsetMinutes,
  bool &parseOk
)
{
  response = "";
  statusCode = -1;
  serverTimeUtc = "";
  deviceType = "";
  timezoneOffsetMinutes = 0;
  parseOk = false;

  bool ok = httpDeviceApi.getConfig(response, statusCode);

  if (!ok)
  {
    Serial.print("Config HTTP status: ");
    Serial.println(statusCode);
    Serial.println(response);
    return false;
  }

  parseOk = parseConfigResponse(
    response,
    doc,
    config,
    serverTimeUtc,
    deviceType,
    timezoneOffsetMinutes
  );

  return parseOk;
}

bool ConfigRuntime::loadCachedConfig(
  FountainConfig &fountainConfig,
  FountainOutputState &outputs,
  FountainDailyTimeline &dailyTimeline,
  String &cachedConfigJson,
  bool &cacheExists
)
{
  cachedConfigJson = loadCachedConfigJson();
  cacheExists = cachedConfigJson.length() > 0;

  if (!cacheExists)
  {
    return false;
  }

  return fountainConfig.loadFromConfigObjectJson(
    cachedConfigJson,
    outputs,
    dailyTimeline
  );
}
