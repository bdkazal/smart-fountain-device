#include "FountainApp.h"
#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#include "ApiClient.h"
#include "ApiHealth.h"
#include "CommandRuntime.h"
#include "CommandFlowRuntime.h"
#include "CloudRuntime.h"
#include "ConfigCache.h"
#include "ConfigRuntime.h"
#include "ConfigFlowRuntime.h"
#include "DailyTimelineRuntime.h"
#include "DeviceClock.h"
#include "DeviceSecrets.h"
#include "DeviceStorage.h"
#include "FountainConfig.h"
#include "FountainOutputs.h"
#include "HttpDeviceApi.h"
#include "FountainTypes.h"
#include "HardwareOutputs.h"
#include "LocalControls.h"
#include "LocalRuntime.h"
#include "SetupPortal.h"
#include "StateSyncRuntime.h"
#include "StateSyncFlowRuntime.h"
#include "StatusIndicators.h"
#include "WaterLevelSensor.h"
#include "WifiReset.h"
#include "WifiRuntime.h"

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
const unsigned long TIMELINE_EVALUATE_INTERVAL_MS = 5000;
const unsigned long NETWORK_TASK_INTERVAL_MS = 50;
const uint32_t NETWORK_TASK_STACK_WORDS = 12288;
const UBaseType_t NETWORK_TASK_PRIORITY = 1;
const int CONFIG_FETCH_MAX_ATTEMPTS = 1;
const int API_OFFLINE_FAILURE_THRESHOLD = 3;

unsigned long lastConfigFetchAt = 0;
unsigned long lastStateSyncAt = 0;
unsigned long lastCommandPollAt = 0;
unsigned long lastWifiRetryAt = 0;
unsigned long lastTimelineEvaluateAt = 0;

TaskHandle_t networkTaskHandle = nullptr;
volatile bool networkTaskEnabled = false;

ApiHealth apiHealth;
CloudRuntime cloudRuntime;
bool outputStateTrusted = false;
String serverTimeUtc = "";
String deviceType = "";

ApiClient apiClient;
HttpDeviceApi httpDeviceApi;
CommandRuntime commandRuntime;
CommandFlowRuntime commandFlowRuntime;
DeviceClock deviceClock;
ConfigRuntime configRuntime;
ConfigFlowRuntime configFlowRuntime;
DailyTimelineRuntime dailyTimelineRuntime;
FountainConfig fountainConfig;
FountainOutputState outputs;
FountainReadings readings;
FountainDailyTimeline dailyTimeline;
WaterLevelSensor waterLevelSensor;
FountainOutputs fountainOutputs;
HardwareOutputs hardwareOutputs;
LocalControls localControls;
LocalRuntime localRuntime;
StateSyncRuntime stateSyncRuntime;
StateSyncFlowRuntime stateSyncFlowRuntime;
StatusIndicators statusIndicators;
WifiRuntime wifiRuntime;

void updateWaterReadings();
void syncHardwareOutputs();
void applySafetyAndSyncHardware();
void updateStatusIndicators();
bool processLocalControls();
bool processDailyTimeline();
bool connectWifi(bool allowDevelopmentFallback = true);
void registerApiSuccess(const char *requestName);
void registerApiFailure(const char *requestName, int statusCode);
bool probeApiRecovery();
void startNetworkTask();
void networkTaskLoop(void *parameter);
void runNetworkRuntimeOnce(unsigned long now);

bool isWifiConnected()
{
  return wifiRuntime.isConnected();
}

bool isOfflineControlMode()
{
  return cloudRuntime.isOfflineControlMode(apiHealth, isWifiConnected());
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

void queueOutputStateSync(const char *reason, const char *source)
{
  markOutputStateTrusted(reason);
  stateSyncRuntime.queueLocalStateSync(source);
}

void queueLocalStateSync(const char *reason)
{
  queueOutputStateSync(reason, "local_button");
}

bool processLocalControls()
{
  updateWaterReadings();

  bool changed = localRuntime.processControls(
    localControls,
    outputs,
    readings
  );

  if (changed)
  {
    dailyTimelineRuntime.markCurrentRangeSatisfied(
      dailyTimeline,
      deviceClock,
      "local control changed output state"
    );

    queueLocalStateSync("local button changed output state");
    applySafetyAndSyncHardware();
  }

  return changed;
}

void logCloudModeIfChanged()
{
  cloudRuntime.logModeIfChanged(apiHealth, isWifiConnected());
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

void updateStatusIndicators()
{
  bool wifiConnected = isWifiConnected();
  bool serverOnline = wifiConnected && !apiHealth.isServerOffline();

  statusIndicators.update(
    wifiConnected,
    serverOnline,
    readings.waterLow,
    isSetupPortalActive()
  );
}

bool loadCachedConfigIfAvailable()
{
  return configFlowRuntime.loadCachedConfigIfAvailable(
    configRuntime,
    fountainConfig,
    outputs,
    dailyTimeline,
    markOutputStateTrusted,
    markOutputStateUntrusted,
    applySafetyAndSyncHardware
  );
}


void serviceRuntimeDuringWifiConnect()
{
  processLocalControls();
  applySafetyAndSyncHardware();
  updateStatusIndicators();
}

bool connectWithCredentials(const String &ssid, const String &password)
{
  return wifiRuntime.connectWithCredentials(
    ssid,
    password,
    serviceRuntimeDuringWifiConnect
  );
}

bool connectWifi(bool allowDevelopmentFallback)
{
  return wifiRuntime.connect(
    allowDevelopmentFallback,
    serviceRuntimeDuringWifiConnect
  );
}

void registerApiSuccess(const char *requestName)
{
  cloudRuntime.registerSuccess(apiHealth, requestName);
}

void registerApiFailure(const char *requestName, int statusCode)
{
  cloudRuntime.registerFailure(apiHealth, requestName, statusCode);
}

bool probeApiRecovery()
{
  return configFlowRuntime.probeApiRecovery(
    isWifiConnected(),
    httpDeviceApi,
    configRuntime,
    CONFIG_FETCH_MAX_ATTEMPTS,
    API_PROBE_HTTP_TIMEOUT_MS,
    registerApiSuccess,
    registerApiFailure
  );
}

bool fetchConfig()
{
  return configFlowRuntime.fetchConfig(
    isWifiConnected(),
    httpDeviceApi,
    configRuntime,
    fountainConfig,
    deviceClock,
    outputs,
    dailyTimeline,
    serverTimeUtc,
    deviceType,
    CONFIG_FETCH_MAX_ATTEMPTS,
    HTTP_TIMEOUT_MS,
    registerApiSuccess,
    registerApiFailure,
    markOutputStateTrusted,
    applySafetyAndSyncHardware
  );
}


bool postState(const char *source = "device_state")
{
  return stateSyncFlowRuntime.postState(
    isWifiConnected(),
    apiHealth.isServerOffline(),
    outputStateTrusted,
    httpDeviceApi,
    stateSyncRuntime,
    outputs,
    readings,
    FIRMWARE_VERSION,
    source,
    updateWaterReadings,
    enforceWaterSafety,
    registerApiSuccess,
    registerApiFailure
  );
}

bool postPendingOutputState()
{
  return stateSyncFlowRuntime.postPendingOutputState(
    isWifiConnected(),
    apiHealth.isServerOffline(),
    outputStateTrusted,
    httpDeviceApi,
    stateSyncRuntime,
    outputs,
    readings,
    FIRMWARE_VERSION,
    updateWaterReadings,
    enforceWaterSafety,
    registerApiSuccess,
    registerApiFailure
  );
}

bool syncLocalStateIfDue(unsigned long now)
{
  return stateSyncFlowRuntime.syncLocalStateIfDue(
    now,
    LOCAL_STATE_SYNC_RETRY_MS,
    isWifiConnected(),
    apiHealth.isServerOffline(),
    stateSyncRuntime,
    postPendingOutputState,
    lastStateSyncAt
  );
}


bool processDailyTimeline()
{
  updateWaterReadings();

  bool cloudAvailable = isWifiConnected() && !apiHealth.isServerOffline();

  DailyTimelineResult result = dailyTimelineRuntime.update(
    dailyTimeline,
    deviceClock,
    cloudAvailable,
    outputs,
    readings,
    fountainOutputs
  );

  if (!result.applied)
  {
    return false;
  }

  queueOutputStateSync(result.reason, result.stateSource);
  applySafetyAndSyncHardware();

  return true;
}

bool pollCommands()
{
  return commandFlowRuntime.pollAndProcess(
    millis(),
    NO_COMMAND_LOG_INTERVAL_MS,
    isWifiConnected(),
    apiHealth.isServerOffline(),
    httpDeviceApi,
    commandRuntime,
    fountainOutputs,
    dailyTimelineRuntime,
    deviceClock,
    dailyTimeline,
    outputs,
    readings,
    stateSyncRuntime,
    registerApiSuccess,
    registerApiFailure,
    updateWaterReadings,
    applySafetyAndSyncHardware,
    markOutputStateTrusted,
    postState
  );
}

void runNetworkRuntimeOnce(unsigned long now)
{
  if (isSetupPortalActive())
  {
    return;
  }

  if (!isWifiConnected())
  {
    if (now - lastWifiRetryAt >= WIFI_RETRY_INTERVAL_MS)
    {
      Serial.println("Wi-Fi offline. Retrying connection from network task...");
      connectWifi(true);
      lastWifiRetryAt = now;
    }

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
    return;
  }

  if (syncLocalStateIfDue(now))
  {
    logCloudModeIfChanged();
    return;
  }

  if (now - lastCommandPollAt >= COMMAND_POLL_INTERVAL_MS)
  {
    bool commandProcessed = pollCommands();
    logCloudModeIfChanged();
    lastCommandPollAt = now;

    if (commandProcessed)
    {
      return;
    }
  }

  if (now - lastStateSyncAt >= STATE_SYNC_INTERVAL_MS)
  {
    postState(stateSyncRuntime.currentOutputSource());
    logCloudModeIfChanged();
    lastStateSyncAt = now;
  }

  if (now - lastConfigFetchAt >= CONFIG_FETCH_INTERVAL_MS)
  {
    fetchConfig();
    logCloudModeIfChanged();
    lastConfigFetchAt = now;
  }
}

void networkTaskLoop(void *parameter)
{
  (void)parameter;

  Serial.println("Network/API task started. Main loop remains responsible for buttons, safety, and RGB animation.");

  for (;;)
  {
    if (networkTaskEnabled)
    {
      runNetworkRuntimeOnce(millis());
    }

    vTaskDelay(pdMS_TO_TICKS(NETWORK_TASK_INTERVAL_MS));
  }
}

void startNetworkTask()
{
  if (networkTaskHandle != nullptr)
  {
    networkTaskEnabled = true;
    return;
  }

  BaseType_t result = xTaskCreatePinnedToCore(
    networkTaskLoop,
    "network_api",
    NETWORK_TASK_STACK_WORDS,
    nullptr,
    NETWORK_TASK_PRIORITY,
    &networkTaskHandle,
    0
  );

  if (result == pdPASS)
  {
    networkTaskEnabled = true;
    Serial.println("Network/API task created on core 0.");
  }
  else
  {
    networkTaskHandle = nullptr;
    networkTaskEnabled = false;
    Serial.println("Network/API task creation failed. API runtime will not run in background.");
  }
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
  statusIndicators.begin();
  waterLevelSensor.begin();
  localControls.begin();
  updateWaterReadings();
  updateStatusIndicators();
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
    updateStatusIndicators();
    return;
  }

  loadCachedConfigIfAvailable();
  connectWifi(true);

  if (isWifiConnected())
  {
    bool configFetched = fetchConfig();

    unsigned long now = millis();
    lastConfigFetchAt = now;
    lastStateSyncAt = now;
    lastCommandPollAt = now;
    lastTimelineEvaluateAt = now;

    if (configFetched && processDailyTimeline())
    {
      syncLocalStateIfDue(now);
    }
    else
    {
      postState("boot");
    }

    pollCommands();
  }

  startNetworkTask();

  Serial.println("Local controls, water safety, and RGB animation are active in the main loop.");
  Serial.println("Laravel API/config/commands/state sync are handled by the background network task.");
  logCloudModeIfChanged();
  updateStatusIndicators();
}

void FountainApp::update()
{
  unsigned long now = millis();

  processLocalControls();

  if (isSetupPortalActive())
  {
    updateWaterReadings();
    updateStatusIndicators();
    handleSetupPortal();
    delay(20);
    return;
  }

  updateWaterReadings();
  enforceWaterSafety();
  logCloudModeIfChanged();
  updateStatusIndicators();

  if (lastTimelineEvaluateAt == 0 || now - lastTimelineEvaluateAt >= TIMELINE_EVALUATE_INTERVAL_MS)
  {
    processDailyTimeline();
    lastTimelineEvaluateAt = now;
  }

  delay(20);
}
