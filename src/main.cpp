#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "ApiClient.h"
#include "ConfigCache.h"
#include "DeviceSecrets.h"
#include "FountainConfig.h"
#include "FountainOutputs.h"
#include "FountainTypes.h"
#include "WaterLevelSensor.h"

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "smart-fountain-dev-0.1"
#endif

// Smart Fountain firmware skeleton.
//
// Main owns the boot/runtime API flow. Product rules that deserve their own
// isolated home are moving out of this file step by step:
// - WaterLevelSensor owns water reading/simulation for now.
// - FountainOutputs owns pump/COB/RGB state application and local pump safety.
// - FountainConfig owns parsing Laravel config.outputs and daily timeline logs.
// - ConfigCache owns the last valid Laravel config stored in ESP32 flash/NVS.
// - ApiClient owns common Laravel URL/header rules.
// Later, offline timeline and UTC/RTC time should become modules too.

const unsigned long CONFIG_FETCH_INTERVAL_MS = 60000;
const unsigned long STATE_SYNC_INTERVAL_MS = 5000;
const unsigned long COMMAND_POLL_INTERVAL_MS = 5000;
const unsigned long WIFI_RETRY_INTERVAL_MS = 10000;

unsigned long lastConfigFetchAt = 0;
unsigned long lastStateSyncAt = 0;
unsigned long lastCommandPollAt = 0;
unsigned long lastWifiRetryAt = 0;

bool serverReachableRecently = false;
String serverTimeUtc = "";
String deviceType = "";

ApiClient apiClient;
FountainConfig fountainConfig;
FountainOutputState outputs;
FountainReadings readings;
WaterLevelSensor waterLevelSensor;
FountainOutputs fountainOutputs;

bool isWifiConnected()
{
  return WiFi.status() == WL_CONNECTED;
}

void updateWaterReadings()
{
  waterLevelSensor.update(readings);
}

void enforceWaterSafety()
{
  fountainOutputs.enforceWaterSafety(outputs, readings);
}

void loadCachedConfigIfAvailable()
{
  String cachedConfigJson = loadCachedConfigJson();

  if (cachedConfigJson.length() == 0)
  {
    Serial.println("No cached Laravel config found.");
    return;
  }

  Serial.println();
  Serial.println("Loading cached Laravel config from flash...");

  bool parsed = fountainConfig.loadFromConfigObjectJson(cachedConfigJson, outputs);

  if (!parsed)
  {
    Serial.println("Cached Laravel config exists but could not be parsed.");
    return;
  }

  Serial.println("Cached Laravel config loaded.");
}

void connectWifi()
{
  Serial.println();
  Serial.print("Connecting Wi-Fi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startedAt = millis();

  while (!isWifiConnected() && millis() - startedAt < 15000)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (isWifiConnected())
  {
    Serial.print("Wi-Fi connected. IP: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("Wi-Fi connection failed. Device will retry later.");
  }
}

bool fetchConfig()
{
  if (!isWifiConnected())
  {
    Serial.println("Config fetch skipped: Wi-Fi offline.");
    return false;
  }

  String url = apiClient.url("/api/device/config?device_uuid=" + String(DEVICE_UUID));

  HTTPClient http;
  http.setTimeout(7000);
  http.begin(url);
  apiClient.addDeviceHeaders(http);

  Serial.println();
  Serial.print("GET ");
  Serial.println(url);

  int statusCode = http.GET();
  String response = http.getString();
  http.end();

  Serial.print("Config HTTP status: ");
  Serial.println(statusCode);

  if (statusCode < 200 || statusCode >= 300)
  {
    Serial.println(response);
    serverReachableRecently = false;
    return false;
  }

  StaticJsonDocument<12288> doc;
  DeserializationError error = deserializeJson(doc, response);

  if (error)
  {
    Serial.print("Config JSON parse failed: ");
    Serial.println(error.c_str());
    serverReachableRecently = false;
    return false;
  }

  serverReachableRecently = true;
  serverTimeUtc = doc["server_time_utc"] | "";

  JsonObject config = doc["config"].as<JsonObject>();
  deviceType = config["device_type"] | "";

  Serial.print("server_time_utc: ");
  Serial.println(serverTimeUtc.length() ? serverTimeUtc : "missing");

  Serial.print("device_type: ");
  Serial.println(deviceType.length() ? deviceType : "missing");

  Serial.print("timezone_offset_minutes: ");
  Serial.println((int)(config["timezone_offset_minutes"] | 0));

  if (deviceType != "smart_fountain")
  {
    Serial.println("WARNING: config.device_type is not smart_fountain.");
  }

  fountainConfig.loadInitialOutputs(config, outputs);
  fountainConfig.printDailyTimeline(config["daily_timeline"].as<JsonObject>());

  // Cache only the inner config object. Reuse the already parsed JSON object so
  // we do not allocate/parse a second large JSON document on the ESP32-C3 stack.
  String configJson;
  serializeJson(config, configJson);

  if (configJson.length() > 0)
  {
    saveCachedConfigJsonIfChanged(configJson);
  }

  return true;
}

String buildStatePayload(const char *source)
{
  // /api/device/state is the trusted hardware truth for persistent-state devices.
  // The backend may know the requested state, but the firmware reports actual state.
  StaticJsonDocument<2048> doc;

  doc["device_uuid"] = DEVICE_UUID;
  doc["device_type"] = "smart_fountain";
  doc["firmware_version"] = FIRMWARE_VERSION;
  doc["operation_state"] = "running";

  JsonObject outputJson = doc.createNestedObject("outputs");

  JsonObject pump = outputJson.createNestedObject("pump");
  pump["enabled"] = outputs.pumpEnabled;
  pump["speed_percent"] = outputs.pumpSpeedPercent;
  pump["source"] = readings.waterLow ? "water_safety" : source;

  JsonObject cob = outputJson.createNestedObject("cob_light");
  cob["enabled"] = outputs.cobEnabled;
  cob["brightness_percent"] = outputs.cobBrightnessPercent;
  cob["source"] = source;

  JsonObject rgb = outputJson.createNestedObject("rgb_light");
  rgb["enabled"] = outputs.rgbEnabled;
  rgb["brightness_percent"] = outputs.rgbBrightnessPercent;
  rgb["color"] = outputs.rgbColor;
  rgb["effect"] = outputs.rgbEffect;
  rgb["source"] = source;

  JsonObject readingsJson = doc.createNestedObject("readings");

  JsonObject waterLow = readingsJson.createNestedObject("water_low");
  waterLow["value"] = readings.waterLow ? 1 : 0;
  waterLow["unit"] = "boolean";

  JsonObject waterLevelPercent = readingsJson.createNestedObject("water_level_percent");
  waterLevelPercent["value"] = readings.waterLevelPercent;
  waterLevelPercent["unit"] = "percent";

  JsonObject waterLevelRaw = readingsJson.createNestedObject("water_level_raw");
  waterLevelRaw["value"] = readings.waterLevelRaw;
  waterLevelRaw["unit"] = "adc";

  String payload;
  serializeJson(doc, payload);

  return payload;
}

bool postState(const char *source = "device_state")
{
  if (!isWifiConnected())
  {
    Serial.println("State sync skipped: Wi-Fi offline.");
    return false;
  }

  updateWaterReadings();
  enforceWaterSafety();

  String url = apiClient.url("/api/device/state");
  String payload = buildStatePayload(source);

  HTTPClient http;
  http.setTimeout(7000);
  http.begin(url);
  apiClient.addDeviceHeaders(http);

  Serial.println();
  Serial.print("POST ");
  Serial.println(url);
  Serial.print("State payload: ");
  Serial.println(payload);

  int statusCode = http.POST(payload);
  String response = http.getString();
  http.end();

  Serial.print("State HTTP status: ");
  Serial.println(statusCode);

  if (statusCode < 200 || statusCode >= 300)
  {
    Serial.println(response);
    serverReachableRecently = false;
    return false;
  }

  serverReachableRecently = true;
  Serial.println("State synced.");
  return true;
}

bool ackCommand(int commandId, const char *status, const char *message = nullptr)
{
  if (!isWifiConnected())
  {
    Serial.println("Command ACK skipped: Wi-Fi offline.");
    return false;
  }

  String url = apiClient.url("/api/device/commands/" + String(commandId) + "/ack");

  StaticJsonDocument<512> doc;
  doc["device_uuid"] = DEVICE_UUID;
  doc["status"] = status;

  if (message != nullptr)
  {
    doc["message"] = message;
  }

  String payload;
  serializeJson(doc, payload);

  HTTPClient http;
  http.setTimeout(7000);
  http.begin(url);
  apiClient.addDeviceHeaders(http);

  Serial.print("ACK command ");
  Serial.print(commandId);
  Serial.print(" as ");
  Serial.println(status);

  int statusCode = http.POST(payload);
  String response = http.getString();
  http.end();

  Serial.print("ACK HTTP status: ");
  Serial.println(statusCode);

  if (statusCode < 200 || statusCode >= 300)
  {
    Serial.println(response);
    serverReachableRecently = false;
    return false;
  }

  serverReachableRecently = true;
  return true;
}

bool handleOutputSet(JsonObject payload)
{
  const char *outputKey = payload["output"] | "";
  JsonObject state = payload["state"].as<JsonObject>();
  const char *source = payload["source"] | "dashboard";

  if (strlen(outputKey) == 0 || state.isNull())
  {
    Serial.println("Invalid output_set payload.");
    return false;
  }

  updateWaterReadings();
  return fountainOutputs.applyOutput(outputKey, state, source, outputs, readings);
}

bool handleSceneApply(JsonObject payload)
{
  JsonObject sceneOutputs = payload["outputs"].as<JsonObject>();
  const char *source = payload["source"] | "scene_apply";

  if (sceneOutputs.isNull())
  {
    Serial.println("Invalid scene_apply payload: outputs missing.");
    return false;
  }

  bool allApplied = true;
  updateWaterReadings();

  for (JsonPair outputPair : sceneOutputs)
  {
    const char *outputKey = outputPair.key().c_str();
    JsonObject state = outputPair.value().as<JsonObject>();

    if (!fountainOutputs.applyOutput(outputKey, state, source, outputs, readings))
    {
      allApplied = false;
    }
  }

  enforceWaterSafety();
  return allApplied;
}

void processCommand(JsonObject command)
{
  int commandId = command["id"] | 0;
  const char *commandType = command["command_type"] | "";
  JsonObject payload = command["payload"].as<JsonObject>();

  if (commandId <= 0 || strlen(commandType) == 0)
  {
    Serial.println("Invalid command shape. Ignoring.");
    return;
  }

  Serial.println();
  Serial.print("Processing command #");
  Serial.print(commandId);
  Serial.print(" type=");
  Serial.println(commandType);

  // For persistent-state products, executed means "state applied", not
  // "operation completed". The final POST /api/device/state remains truth.
  ackCommand(commandId, "acknowledged");

  bool applied = false;
  String type = commandType;

  if (type == "output_set")
  {
    applied = handleOutputSet(payload);
  }
  else if (type == "scene_apply")
  {
    applied = handleSceneApply(payload);
  }
  else
  {
    ackCommand(commandId, "failed", "Unsupported command type.");
    return;
  }

  if (applied)
  {
    ackCommand(commandId, "executed");
  }
  else
  {
    ackCommand(commandId, "failed", "Command could not be applied safely.");
  }

  postState("device_state");
}

bool pollCommands()
{
  if (!isWifiConnected())
  {
    Serial.println("Command poll skipped: Wi-Fi offline.");
    return false;
  }

  String url = apiClient.url("/api/device/commands?device_uuid=" + String(DEVICE_UUID));

  HTTPClient http;
  http.setTimeout(7000);
  http.begin(url);
  apiClient.addDeviceHeaders(http);

  Serial.println();
  Serial.print("GET ");
  Serial.println(url);

  int statusCode = http.GET();
  String response = http.getString();
  http.end();

  Serial.print("Command HTTP status: ");
  Serial.println(statusCode);

  if (statusCode < 200 || statusCode >= 300)
  {
    Serial.println(response);
    serverReachableRecently = false;
    return false;
  }

  serverReachableRecently = true;

  StaticJsonDocument<8192> doc;
  DeserializationError error = deserializeJson(doc, response);

  if (error)
  {
    Serial.print("Command JSON parse failed: ");
    Serial.println(error.c_str());
    return false;
  }

  JsonObject command = doc["command"].as<JsonObject>();

  if (command.isNull())
  {
    Serial.println("No pending command.");
    return true;
  }

  processCommand(command);
  return true;
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Biztola Smart Fountain ESP32 starting...");
  Serial.print("Firmware version: ");
  Serial.println(FIRMWARE_VERSION);

  beginConfigCache();
  apiClient.begin(API_BASE_URL, DEVICE_API_KEY);
  waterLevelSensor.begin();
  updateWaterReadings();

  // Cached config gives the device a useful last-known output/timeline state
  // before Laravel is contacted. This is the foundation for offline timeline.
  loadCachedConfigIfAvailable();

  connectWifi();

  if (isWifiConnected())
  {
    fetchConfig();
    postState("boot");
    pollCommands();

    unsigned long now = millis();
    lastConfigFetchAt = now;
    lastStateSyncAt = now;
    lastCommandPollAt = now;
  }
}

void loop()
{
  unsigned long now = millis();

  // Local readings and safety run before network work so slow/failing HTTP
  // cannot delay pump protection.
  updateWaterReadings();
  enforceWaterSafety();

  if (!isWifiConnected())
  {
    if (now - lastWifiRetryAt >= WIFI_RETRY_INTERVAL_MS)
    {
      Serial.println("Wi-Fi offline. Retrying connection...");
      connectWifi();
      lastWifiRetryAt = now;
    }

    delay(20);
    return;
  }

  if (now - lastConfigFetchAt >= CONFIG_FETCH_INTERVAL_MS)
  {
    fetchConfig();
    lastConfigFetchAt = now;
  }

  if (now - lastStateSyncAt >= STATE_SYNC_INTERVAL_MS)
  {
    postState("device_state");
    lastStateSyncAt = now;
  }

  if (now - lastCommandPollAt >= COMMAND_POLL_INTERVAL_MS)
  {
    pollCommands();
    lastCommandPollAt = now;
  }

  delay(20);
}
