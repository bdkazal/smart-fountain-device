#include "ConfigRuntime.h"

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
