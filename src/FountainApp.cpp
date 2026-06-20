#include "FountainApp.h"
#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#include "ApiClient.h"
#include "ApiHealth.h"
#include "CommandRuntime.h"
#include "ConfigCache.h"
#include "ConfigRuntime.h"
#include "DeviceClock.h"
#include "DeviceSecrets.h"
#include "DeviceStorage.h"
#include "FountainConfig.h"
#include "FountainOutputs.h"
#include "HttpDeviceApi.h"
#include "FountainTypes.h"
#include "HardwareOutputs.h"
#include "LocalControls.h"
#include "SetupPortal.h"
#include "StateSyncRuntime.h"
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
// - local buttons and water safety continue to work
// - do not apply daily timeline ranges or scene presets automatically
// - do not change outputs only because Laravel disappeared
//
// Reboot while offline:
// - load saved compact config from flash
// - restore that saved output state
// - keep that state until Laravel becomes reachable again
//
// GPIO33 setup reset is the only exception: setup mode clears Wi-Fi/cache and
// forces outputs OFF for safety.

const unsigned long CONFIG_FETCH_INTERVAL_MS = 120000;
const unsigned long STATE_SYNC_INTERVAL_MS = 10000;
const unsigned long COMMAND_POLL_INTERVAL_MS = 2000;
const unsigned long WIFI_RETRY_INTERVAL_MS = 10000;
const unsigned long HTTP_TIMEOUT_MS = 5000;
const unsigned long COMMAND_HTTP_TIMEOUT_MS = 5000;
// Used only while already in API-offline mode. Keep normal requests longer for remote hosting,
// but keep failed recovery probes shorter so local buttons do not feel frozen.
const unsigned long API_PROBE_HTTP_TIMEOUT_MS = 2500;
const unsigned long NO_COMMAND_LOG_INTERVAL_MS = 30000;
const unsigned long LOCAL_STATE_SYNC_RETRY_MS = 5000;
const unsigned long API_PROBE_INTERVAL_MS = 30000;
const int CONFIG_FETCH_MAX_ATTEMPTS = 1;
const int API_OFFLINE_FAILURE_THRESHOLD = 3;

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
unsigned long lastNoCommandLogAt = 0;

ApiHealth apiHealth;
bool outputStateTrusted = false;
String serverTimeUtc = "";
String deviceType = "";
CloudControlMode lastLoggedCloudMode = CLOUD_MODE_UNKNOWN;

ApiClient apiClient;
HttpDeviceApi httpDeviceApi;
CommandRuntime commandRuntime;
DeviceClock deviceClock;
ConfigRuntime configRuntime;
FountainConfig fountainConfig;
FountainOutputState outputs;
FountainReadings readings;
FountainDailyTimeline dailyTimeline;
WaterLevelSensor waterLevelSensor;
FountainOutputs fountainOutputs;
HardwareOutputs hardwareOutputs;
LocalControls localControls;
StateSyncRuntime stateSyncRuntime;

void updateWaterReadings();
void syncHardwareOutputs();
void applySafetyAndSyncHardware();
bool processLocalControls();
bool connectWifi(bool allowDevelopmentFallback = true);
void registerApiSuccess(const char *requestName);
void registerApiFailure(const char *requestName, int statusCode);
bool shouldCountApiFailureForOffline(const char *requestName);
bool probeApiRecovery();

bool isWifiConnected()
{
  return WiFi.status() == WL_CONNECTED;
}

bool isOfflineControlMode()
{
  return !isWifiConnected() || apiHealth.isServerOffline();
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

void queueLocalStateSync(const char *reason)
{
  markOutputStateTrusted(reason);
  stateSyncRuntime.queueLocalStateSync();
}

bool processLocalControls()
{
  localControls.update();

  bool changed = false;

  if (localControls.consumePumpToggleRequest())
  {
    updateWaterReadings();

    bool requestedEnabled = !outputs.pumpEnabled;

    if (requestedEnabled && readings.waterLow)
    {
      outputs.pumpEnabled = false;
      outputs.pumpSpeedPercent = 0;
      Serial.println("Local pump button ignored because water_low=true.");
    }
    else
    {
      outputs.pumpEnabled = requestedEnabled;
      outputs.pumpSpeedPercent = requestedEnabled ? 100 : 0;
      changed = true;

      Serial.print("Pump state applied from local_button: enabled=");
      Serial.print(outputs.pumpEnabled ? "true" : "false");
      Serial.println(" mode=on_off");
    }
  }

  if (localControls.consumeCobToggleRequest())
  {
    outputs.cobEnabled = !outputs.cobEnabled;
    outputs.cobBrightnessPercent = outputs.cobEnabled ? 100 : 0;
    changed = true;

    Serial.print("COB state applied from local_button: enabled=");
    Serial.print(outputs.cobEnabled ? "true" : "false");
    Serial.println(" mode=on_off");
  }

  if (changed)
  {
    queueLocalStateSync("local button changed output state");
    applySafetyAndSyncHardware();
  }

  return changed;
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
    Serial.println("Cloud mode: OFFLINE - Wi-Fi disconnected, local buttons/water safety active, keeping last trusted saved output state.");
  }
  else
  {
    Serial.println("Cloud mode: OFFLINE - API unavailable, local buttons/water safety active, keeping last trusted saved output state.");
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
  String cachedConfigJson;
  bool cacheExists = false;

  bool parsed = configRuntime.loadCachedConfig(
    fountainConfig,
    outputs,
    dailyTimeline,
    cachedConfigJson,
    cacheExists
  );

  if (!cacheExists)
  {
    Serial.println("No cached Laravel config found.");
    markOutputStateUntrusted("no cached Laravel config; safe boot OFF must not sync to Laravel");
    return false;
  }

  Serial.println();
  Serial.print("Loading cached Laravel config from flash. bytes=");
  Serial.println(cachedConfigJson.length());

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
    processLocalControls();
    applySafetyAndSyncHardware();
    delay(100);
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

bool connectWifi(bool allowDevelopmentFallback)
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

  Serial.println("Wi-Fi unavailable. Will retry later. Setup hotspot starts only after GPIO33 Wi-Fi reset.");
  return false;
}

bool shouldCountApiFailureForOffline(const char *requestName)
{
  String request = requestName;
  return request == "commands" || request == "state" || request == "config";
}

void registerApiSuccess(const char *requestName)
{
  apiHealth.registerSuccess(requestName);
}

void registerApiFailure(const char *requestName, int statusCode)
{
  if (shouldCountApiFailureForOffline(requestName))
  {
    apiHealth.registerFailure(requestName, statusCode);
    return;
  }

  apiHealth.registerWarning(requestName, statusCode);
}

bool getConfigWithRetry(String &response, int &statusCode, unsigned long timeoutMs = HTTP_TIMEOUT_MS)
{
  response = "";
  statusCode = -1;

  for (int attempt = 1; attempt <= CONFIG_FETCH_MAX_ATTEMPTS; attempt++)
  {
    Serial.println();
    Serial.print("GET ");
    Serial.println(httpDeviceApi.configUrl());

    bool ok = httpDeviceApi.getConfigWithTimeout(timeoutMs, response, statusCode);

    Serial.print("Config HTTP status: ");
    Serial.println(statusCode);

    if (ok)
    {
      return true;
    }

    if (attempt < CONFIG_FETCH_MAX_ATTEMPTS)
    {
      delay(100);
    }
  }

  return false;
}

bool probeApiRecovery()
{
  if (!isWifiConnected())
  {
    Serial.println("API recovery probe skipped: Wi-Fi offline.");
    return false;
  }

  String response;
  int statusCode;

  if (!getConfigWithRetry(response, statusCode, API_PROBE_HTTP_TIMEOUT_MS))
  {
    if (response.length() > 0)
    {
      Serial.println(response);
    }

    registerApiFailure("config", statusCode);
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, response);

  if (error)
  {
    Serial.print("API recovery probe JSON parse failed: ");
    Serial.println(error.c_str());
    registerApiFailure("config_parse", statusCode);
    return false;
  }

  registerApiSuccess("config_probe");
  return true;
}

bool fetchConfig()
{
  if (!isWifiConnected())
  {
    Serial.println("Config fetch skipped: Wi-Fi offline.");
    return false;
  }

  String response;
  int statusCode;

  if (!getConfigWithRetry(response, statusCode))
  {
    if (response.length() > 0)
    {
      Serial.println(response);
    }

    registerApiFailure("config", statusCode);
    return false;
  }

  JsonDocument doc;
  JsonObject config;
  int timezoneOffsetMinutes = 0;

  if (!configRuntime.parseConfigResponse(response, doc, config, serverTimeUtc, deviceType, timezoneOffsetMinutes))
  {
    registerApiFailure("config_parse", statusCode);
    return false;
  }

  registerApiSuccess("config");

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

bool postState(const char *source = "device_state")
{
  if (!isWifiConnected())
  {
    Serial.println("State sync skipped: Wi-Fi offline.");
    return false;
  }

  if (apiHealth.isServerOffline())
  {
    Serial.println("State sync skipped: API offline mode.");
    return false;
  }

  if (!outputStateTrusted)
  {
    Serial.println("State sync skipped: output state is not trusted yet. Safe boot OFF will not be pushed to Laravel.");
    return false;
  }

  updateWaterReadings();
  enforceWaterSafety();

  String response;
  int statusCode;

  stateSyncRuntime.postState(
    httpDeviceApi,
    source,
    outputs,
    readings,
    FIRMWARE_VERSION,
    response,
    statusCode
  );

  Serial.print("State HTTP status: ");
  Serial.println(statusCode);

  if (statusCode < 200 || statusCode >= 300)
  {
    Serial.println(response);
    registerApiFailure("state", statusCode);
    return false;
  }

  registerApiSuccess("state");
  Serial.println("State synced.");
  return true;
}

bool syncLocalStateIfDue(unsigned long now)
{
  if (!stateSyncRuntime.hasPendingLocalSync())
  {
    return false;
  }

  if (!isWifiConnected())
  {
    return false;
  }

  if (apiHealth.isServerOffline())
  {
    return false;
  }

  if (!stateSyncRuntime.shouldSyncLocalState(now, LOCAL_STATE_SYNC_RETRY_MS))
  {
    return false;
  }

  Serial.println("Syncing local button output change to Laravel...");

  if (postState("local_button"))
  {
    stateSyncRuntime.markLocalStateSynced();
    lastStateSyncAt = now;
    return true;
  }

  stateSyncRuntime.markLocalStateSyncFailed(now);
  return false;
}

bool ackCommand(int commandId, const char *status, const char *message = nullptr)
{
  if (!isWifiConnected())
  {
    Serial.println("Command ACK skipped: Wi-Fi offline.");
    return false;
  }

  String response;
  int statusCode;

  commandRuntime.ackCommand(
    httpDeviceApi,
    commandId,
    status,
    message,
    response,
    statusCode
  );

  if (statusCode < 200 || statusCode >= 300)
  {
    Serial.println(response);
    registerApiFailure("ack", statusCode);
    return false;
  }

  registerApiSuccess("ack");
  return true;
}

bool handleOutputSet(JsonObject payload)
{
  updateWaterReadings();

  bool applied = commandRuntime.handleOutputSet(
    payload,
    fountainOutputs,
    outputs,
    readings
  );

  applySafetyAndSyncHardware();
  return applied;
}

bool handleSceneApply(JsonObject payload)
{
  updateWaterReadings();

  bool allApplied = commandRuntime.handleSceneApply(
    payload,
    fountainOutputs,
    outputs,
    readings
  );

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

  if (apiHealth.isServerOffline())
  {
    return false;
  }

  JsonDocument doc;
  String response;
  int statusCode;
  bool parseOk = false;

  bool fetched = commandRuntime.fetchCommand(
    httpDeviceApi,
    doc,
    response,
    statusCode,
    parseOk
  );

  if (statusCode < 200 || statusCode >= 300)
  {
    registerApiFailure("commands", statusCode);
    return false;
  }

  registerApiSuccess("commands");

  if (!fetched || !parseOk)
  {
    registerApiFailure("commands_parse", statusCode);
    return false;
  }

  JsonObject command = doc["command"].as<JsonObject>();

  if (command.isNull())
  {
    unsigned long now = millis();

    if (now - lastNoCommandLogAt >= NO_COMMAND_LOG_INTERVAL_MS)
    {
      Serial.println("No pending command.");
      lastNoCommandLogAt = now;
    }

    return false;
  }

  Serial.println();
  Serial.print("GET ");
  Serial.println(httpDeviceApi.commandsUrl());
  Serial.print("Command HTTP status: ");
  Serial.println(statusCode);

  processCommand(command);
  return true;
}

void FountainApp::begin()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Biztola Smart Fountain ESP32 starting...");
  Serial.print("Firmware version: ");
  Serial.println(FIRMWARE_VERSION);

  beginConfigCache();
  beginDeviceStorage();
  apiHealth.begin(API_OFFLINE_FAILURE_THRESHOLD, API_PROBE_INTERVAL_MS);
  apiClient.begin(API_BASE_URL, DEVICE_API_KEY);
  httpDeviceApi.begin(&apiClient, HTTP_TIMEOUT_MS, COMMAND_HTTP_TIMEOUT_MS);
  hardwareOutputs.begin();
  waterLevelSensor.begin();
  localControls.begin();
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

  Serial.println("Local controls and water safety are active during runtime.");
  logCloudModeIfChanged();
}

void FountainApp::update()
{
  unsigned long now = millis();

  processLocalControls();

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

  if (apiHealth.isServerOffline())
  {
    if (apiHealth.shouldProbe(now))
    {
      apiHealth.markProbeAttempt(now);

      if (stateSyncRuntime.hasPendingLocalSync())
      {
        Serial.println("API offline mode: probing Laravel before local state sync...");

        if (probeApiRecovery())
        {
          logCloudModeIfChanged();
          syncLocalStateIfDue(now);
        }
      }
      else
      {
        Serial.println("API offline mode: probing Laravel...");

        if (fetchConfig())
        {
          lastConfigFetchAt = now;
          logCloudModeIfChanged();
        }
      }
    }

    logCloudModeIfChanged();
    delay(20);
    return;
  }

  if (syncLocalStateIfDue(now))
  {
    logCloudModeIfChanged();
    delay(20);
    return;
  }

  if (now - lastCommandPollAt >= COMMAND_POLL_INTERVAL_MS)
  {
    bool commandProcessed = pollCommands();
    logCloudModeIfChanged();
    lastCommandPollAt = now;

    if (commandProcessed)
    {
      delay(20);
      return;
    }
  }

  if (now - lastStateSyncAt >= STATE_SYNC_INTERVAL_MS)
  {
    postState("device_state");
    logCloudModeIfChanged();
    lastStateSyncAt = now;
  }

  if (now - lastConfigFetchAt >= CONFIG_FETCH_INTERVAL_MS)
  {
    fetchConfig();
    logCloudModeIfChanged();
    lastConfigFetchAt = now;
  }

  delay(20);
}
