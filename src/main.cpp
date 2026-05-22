#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "ApiClient.h"
#include "ConfigCache.h"
#include "DeviceClock.h"
#include "DeviceSecrets.h"
#include "DeviceStorage.h"
#include "FountainConfig.h"
#include "FountainOutputs.h"
#include "FountainTypes.h"
#include "HardwareOutputs.h"
#include "SetupPortal.h"
#include "WaterLevelSensor.h"
#include "WifiReset.h"

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "smart-fountain-dev-0.1"
#endif

// Smart Fountain V1 is a persistent-state device.
//
// Online:
// - fetch Laravel current config/settings
// - apply outputs
// - save compact config if changed
// - keep checking periodically
//
// Offline / server unavailable:
// - keep the last trusted saved/current output state
// - do not apply daily timeline ranges or scene presets automatically
// - do not change outputs only because Laravel disappeared
//
// Reboot while offline:
// - load saved compact config from flash
// - restore that saved output state
// - keep that state until Laravel becomes reachable again
//
// GPIO7 setup reset is the only exception: setup mode clears Wi-Fi/cache and
// forces outputs OFF for safety.

const unsigned long CONFIG_FETCH_INTERVAL_MS = 60000;
const unsigned long STATE_SYNC_INTERVAL_MS = 5000;
const unsigned long COMMAND_POLL_INTERVAL_MS = 5000;
const unsigned long OFFLINE_API_RETRY_INTERVAL_MS = 30000;
const unsigned long WIFI_RETRY_INTERVAL_MS = 10000;
const int CONFIG_FETCH_MAX_ATTEMPTS = 2;

enum CloudControlMode
{
  CLOUD_MODE_UNKNOWN,
  CLOUD_MODE_ONLINE,
  CLOUD_MODE_OFFLINE
};

unsigned long lastConfigFetchAt = 0;
unsigned long lastStateSyncAt = 0;
unsigned long lastCommandPollAt = 0;
unsigned long lastWifiRetryAt = 0;

bool serverReachableRecently = false;
bool outputStateTrusted = false;
String serverTimeUtc = "";
String deviceType = "";
CloudControlMode lastLoggedCloudMode = CLOUD_MODE_UNKNOWN;

ApiClient apiClient;
DeviceClock deviceClock;
FountainConfig fountainConfig;
FountainOutputState outputs;
FountainReadings readings;
FountainDailyTimeline dailyTimeline;
WaterLevelSensor waterLevelSensor;
FountainOutputs fountainOutputs;
HardwareOutputs hardwareOutputs;

void updateWaterReadings();
void syncHardwareOutputs();
void applySafetyAndSyncHardware();

bool isWifiConnected()
{
  return WiFi.status() == WL_CONNECTED;
}

bool isOfflineControlMode()
{
  return !isWifiConnected() || !serverReachableRecently;
}

unsigned long activeApiInterval(unsigned long normalInterval)
{
  return isOfflineControlMode() ? OFFLINE_API_RETRY_INTERVAL_MS : normalInterval;
}

void syncHardwareOutputs()
{
  hardwareOutputs.apply(outputs);
}

void markOutputStateTrusted(const char *reason)
{
  if (!outputStateTrusted)
  {
    Serial.print("Output state trusted: ");
    Serial.println(reason);
  }

  outputStateTrusted = true;
}

void markOutputStateUntrusted(const char *reason)
{
  outputStateTrusted = false;
  Serial.print("Output state untrusted: ");
  Serial.println(reason);
}

void forceAllOutputsOff(const char *reason)
{
  outputs.pumpEnabled = false;
  outputs.pumpSpeedPercent = 0;
  outputs.cobEnabled = false;
  outputs.cobBrightnessPercent = 0;
  outputs.rgbEnabled = false;
  outputs.rgbBrightnessPercent = 0;
  outputs.rgbColor = "#000000";
  outputs.rgbEffect = "solid";

  Serial.print("All outputs forced OFF: ");
  Serial.println(reason);
  syncHardwareOutputs();
}

void applySafetyAndSyncHardware()
{
  updateWaterReadings();
  fountainOutputs.enforceWaterSafety(outputs, readings);
  syncHardwareOutputs();
}

void logCloudModeIfChanged()
{
  CloudControlMode currentMode = isOfflineControlMode() ? CLOUD_MODE_OFFLINE : CLOUD_MODE_ONLINE;

  if (currentMode == lastLoggedCloudMode)
  {
    return;
  }

  lastLoggedCloudMode = currentMode;

  if (currentMode == CLOUD_MODE_ONLINE)
  {
    Serial.println("Cloud mode: ONLINE - Laravel reachable, dashboard/API control active.");
    return;
  }

  if (!isWifiConnected())
  {
    Serial.println("Cloud mode: OFFLINE - Wi-Fi disconnected, keeping last trusted saved output state.");
  }
  else
  {
    Serial.println("Cloud mode: OFFLINE - API unavailable, keeping last trusted saved output state.");
  }
}

void updateWaterReadings()
{
  waterLevelSensor.update(readings);
}

void enforceWaterSafety()
{
  fountainOutputs.enforceWaterSafety(outputs, readings);
  syncHardwareOutputs();
}

bool loadCachedConfigIfAvailable()
{
  String cachedConfigJson = loadCachedConfigJson();

  if (cachedConfigJson.length() == 0)
  {
    Serial.println("No cached Laravel config found.");
    markOutputStateUntrusted("no cached Laravel config; safe boot OFF must not sync to Laravel");
    return false;
  }

  Serial.println();
  Serial.print("Loading cached Laravel config from flash. bytes=");
  Serial.println(cachedConfigJson.length());

  bool parsed = fountainConfig.loadFromConfigObjectJson(cachedConfigJson, outputs, dailyTimeline);

  if (!parsed)
  {
    Serial.println("Cached Laravel config exists but could not be parsed.");
    markOutputStateUntrusted("cached Laravel config parse failed; safe boot OFF must not sync to Laravel");
    return false;
  }

  markOutputStateTrusted("cached Laravel config loaded");
  applySafetyAndSyncHardware();
  Serial.println("Cached Laravel config loaded. Device will keep this state if Laravel is unavailable.");
  return true;
}

bool connectWithCredentials(const String &ssid, const String &password)
{
  if (ssid.length() == 0)
  {
    Serial.println("Cannot connect to Wi-Fi: SSID is empty.");
    return false;
  }

  Serial.println();
  Serial.println("Connecting Wi-Fi...");
  Serial.print("SSID: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.begin(ssid.c_str(), password.c_str());

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
    Serial.print("RSSI dBm: ");
    Serial.println(WiFi.RSSI());
    return true;
  }

  Serial.println("Wi-Fi connection failed.");
  return false;
}

bool connectWifi(bool allowDevelopmentFallback = true)
{
  StoredDeviceConfig storedConfig = loadStoredDeviceConfig();

  if (storedConfig.hasWifiCredentials)
  {
    Serial.println("Trying stored Wi-Fi credentials...");

    if (connectWithCredentials(storedConfig.wifiSsid, storedConfig.wifiPassword))
    {
      return true;
    }

    Serial.println("Stored Wi-Fi failed.");
  }
  else
  {
    Serial.println("No stored Wi-Fi credentials found.");
  }

  if (allowDevelopmentFallback)
  {
    Serial.println("Trying DeviceSecrets Wi-Fi as development fallback...");

    if (connectWithCredentials(WIFI_SSID, WIFI_PASSWORD))
    {
      return true;
    }

    Serial.println("DeviceSecrets Wi-Fi failed.");
  }

  Serial.println("Wi-Fi unavailable. Will retry later. Setup hotspot starts only after GPIO7 Wi-Fi reset.");
  return false;
}

bool getWithRetry(const String &url, String &response, int &statusCode)
{
  response = "";
  statusCode = -1;

  for (int attempt = 1; attempt <= CONFIG_FETCH_MAX_ATTEMPTS; attempt++)
  {
    HTTPClient http;
    http.setTimeout(7000);
    http.begin(url);
    apiClient.addDeviceHeaders(http);

    if (attempt == 1)
    {
      Serial.println();
      Serial.print("GET ");
      Serial.println(url);
    }
    else
    {
      Serial.print("Retry GET attempt ");
      Serial.print(attempt);
      Serial.print(" ");
      Serial.println(url);
    }

    statusCode = http.GET();
    response = http.getString();
    http.end();

    Serial.print("Config HTTP status: ");
    Serial.println(statusCode);

    if (statusCode >= 200 && statusCode < 300)
    {
      return true;
    }

    if (attempt < CONFIG_FETCH_MAX_ATTEMPTS)
    {
      delay(300);
    }
  }

  return false;
}

bool fetchConfig()
{
  if (!isWifiConnected())
  {
    Serial.println("Config fetch skipped: Wi-Fi offline.");
    return false;
  }

  String url = apiClient.url("/api/device/config?device_uuid=" + String(DEVICE_UUID));
  String response;
  int statusCode;

  if (!getWithRetry(url, response, statusCode))
  {
    if (response.length() > 0)
    {
      Serial.println(response);
    }

    serverReachableRecently = false;
    return false;
  }

  JsonDocument doc;
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
  int timezoneOffsetMinutes = config["timezone_offset_minutes"] | 0;

  Serial.print("server_time_utc: ");
  Serial.println(serverTimeUtc.length() ? serverTimeUtc : "missing");

  Serial.print("device_type: ");
  Serial.println(deviceType.length() ? deviceType : "missing");

  Serial.print("timezone_offset_minutes: ");
  Serial.println(timezoneOffsetMinutes);

  deviceClock.syncFromServerTime(serverTimeUtc, timezoneOffsetMinutes);

  if (deviceType != "smart_fountain")
  {
    Serial.println("WARNING: config.device_type is not smart_fountain.");
  }

  fountainConfig.loadInitialOutputs(config, outputs);
  markOutputStateTrusted("fresh Laravel config fetched");
  applySafetyAndSyncHardware();
  fountainConfig.loadDailyTimeline(config["daily_timeline"].as<JsonObject>(), dailyTimeline);
  fountainConfig.printDailyTimeline(dailyTimeline);

  String configJson = fountainConfig.buildCompactCacheJson(config);

  Serial.print("Compact config cache JSON length: ");
  Serial.println(configJson.length());

  if (configJson.length() > 0)
  {
    saveCachedConfigJsonIfChanged(configJson);
  }

  return true;
}

String buildStatePayload(const char *source)
{
  JsonDocument doc;

  doc["device_uuid"] = DEVICE_UUID;
  doc["device_type"] = "smart_fountain";
  doc["firmware_version"] = FIRMWARE_VERSION;
  doc["operation_state"] = "running";

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

bool postState(const char *source = "device_state")
{
  if (!isWifiConnected())
  {
    Serial.println("State sync skipped: Wi-Fi offline.");
    return false;
  }

  if (!outputStateTrusted)
  {
    Serial.println("State sync skipped: output state is not trusted yet. Safe boot OFF will not be pushed to Laravel.");
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

  JsonDocument doc;
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
  bool applied = fountainOutputs.applyOutput(outputKey, state, source, outputs, readings);
  applySafetyAndSyncHardware();
  return applied;
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

  applySafetyAndSyncHardware();
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
    markOutputStateTrusted("cloud command applied");
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

  JsonDocument doc;
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
  beginDeviceStorage();
  apiClient.begin(API_BASE_URL, DEVICE_API_KEY);
  hardwareOutputs.begin();
  waterLevelSensor.begin();
  updateWaterReadings();
  forceAllOutputsOff("safe boot hardware default");
  markOutputStateUntrusted("safe boot OFF is hardware-only until cached or fresh config loads");

  checkWifiResetOnBoot();
  bool wifiSetupRequested = consumeWifiSetupPortalRequest();

  if (wifiSetupRequested)
  {
    Serial.println("Wi-Fi setup was requested. Setup mode will not load cached config or run local timeline.");
    forceAllOutputsOff("Wi-Fi setup mode");
    markOutputStateUntrusted("Wi-Fi setup mode output OFF must not sync to Laravel");
    startSetupPortal();
    logCloudModeIfChanged();
    return;
  }

  loadCachedConfigIfAvailable();
  connectWifi(true);

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

  logCloudModeIfChanged();
}

void loop()
{
  unsigned long now = millis();

  if (isSetupPortalActive())
  {
    handleSetupPortal();
    delay(20);
    return;
  }

  updateWaterReadings();
  enforceWaterSafety();
  logCloudModeIfChanged();

  if (!isWifiConnected())
  {
    if (now - lastWifiRetryAt >= WIFI_RETRY_INTERVAL_MS)
    {
      Serial.println("Wi-Fi offline. Retrying connection...");
      connectWifi(true);
      lastWifiRetryAt = now;
    }

    delay(20);
    return;
  }

  unsigned long configInterval = activeApiInterval(CONFIG_FETCH_INTERVAL_MS);
  unsigned long stateInterval = activeApiInterval(STATE_SYNC_INTERVAL_MS);
  unsigned long commandInterval = activeApiInterval(COMMAND_POLL_INTERVAL_MS);

  if (now - lastConfigFetchAt >= configInterval)
  {
    fetchConfig();
    logCloudModeIfChanged();
    lastConfigFetchAt = now;
  }

  if (now - lastStateSyncAt >= stateInterval)
  {
    postState("device_state");
    logCloudModeIfChanged();
    lastStateSyncAt = now;
  }

  if (now - lastCommandPollAt >= commandInterval)
  {
    pollCommands();
    logCloudModeIfChanged();
    lastCommandPollAt = now;
  }

  delay(20);
}
