#include "StateSyncRuntime.h"

#include <ArduinoJson.h>

#include "DeviceSecrets.h"

String StateSyncRuntime::buildStatePayload(
  const char *source,
  const FountainOutputState &outputs,
  const FountainReadings &readings,
  const char *firmwareVersion
)
{
  JsonDocument doc;

  doc["device_uuid"] = DEVICE_UUID;
  doc["device_type"] = "smart_fountain";
  doc["firmware_version"] = firmwareVersion;
  doc["operation_state"] = readings.waterLow ? "water_low_lockout" : "running";

  JsonObject outputJson = doc["outputs"].to<JsonObject>();

  JsonObject pump = outputJson["pump"].to<JsonObject>();
  pump["enabled"] = outputs.pumpEnabled;
  pump["source"] = readings.waterLow ? "water_safety" : source;

  JsonObject cob = outputJson["cob_light"].to<JsonObject>();
  cob["enabled"] = outputs.cobEnabled;
  cob["source"] = source;

  JsonObject rgb = outputJson["rgb_light"].to<JsonObject>();
  rgb["enabled"] = outputs.rgbEnabled;
  rgb["brightness_percent"] = outputs.rgbBrightnessPercent;
  rgb["color"] = outputs.rgbColor;
  rgb["effect"] = outputs.rgbEffect;
  rgb["source"] = source;

  JsonObject readingsJson = doc["readings"].to<JsonObject>();

  JsonObject waterLow = readingsJson["water_low"].to<JsonObject>();
  waterLow["value"] = readings.waterLow ? 1 : 0;
  waterLow["unit"] = "boolean";

  String payload;
  serializeJson(doc, payload);

  return payload;
}
