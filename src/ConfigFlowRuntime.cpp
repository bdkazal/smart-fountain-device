#include "ConfigFlowRuntime.h"

bool ConfigFlowRuntime::loadCachedConfigIfAvailable(
  ConfigRuntime &configRuntime,
  FountainConfig &fountainConfig,
  FountainOutputState &outputs,
  FountainDailyTimeline &dailyTimeline,
  ConfigFlowMessageCallback markOutputStateTrusted,
  ConfigFlowMessageCallback markOutputStateUntrusted,
  ConfigFlowVoidCallback applySafetyAndSyncHardware
)
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

    if (markOutputStateUntrusted != nullptr)
    {
      markOutputStateUntrusted("no cached Laravel config; safe boot OFF must not sync to Laravel");
    }

    return false;
  }

  Serial.println();
  Serial.print("Loading cached Laravel config from flash. bytes=");
  Serial.println(cachedConfigJson.length());

  if (!parsed)
  {
    Serial.println("Cached Laravel config exists but could not be parsed.");

    if (markOutputStateUntrusted != nullptr)
    {
      markOutputStateUntrusted("cached Laravel config parse failed; safe boot OFF must not sync to Laravel");
    }

    return false;
  }

  if (markOutputStateTrusted != nullptr)
  {
    markOutputStateTrusted("cached Laravel config loaded");
  }

  if (applySafetyAndSyncHardware != nullptr)
  {
    applySafetyAndSyncHardware();
  }

  Serial.println("Cached Laravel config loaded. Device will keep this state if Laravel is unavailable.");
  return true;
}

bool ConfigFlowRuntime::getConfigWithRetry(
  HttpDeviceApi &httpDeviceApi,
  String &response,
  int &statusCode,
  int maxAttempts,
  unsigned long timeoutMs
)
{
  response = "";
  statusCode = -1;

  for (int attempt = 1; attempt <= maxAttempts; attempt++)
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

    if (attempt < maxAttempts)
    {
      delay(100);
    }
  }

  return false;
}

bool ConfigFlowRuntime::probeApiRecovery(
  bool wifiConnected,
  HttpDeviceApi &httpDeviceApi,
  ConfigRuntime &configRuntime,
  int maxAttempts,
  unsigned long timeoutMs,
  ConfigFlowApiSuccessCallback registerApiSuccess,
  ConfigFlowApiFailureCallback registerApiFailure
)
{
  if (!wifiConnected)
  {
    Serial.println("API recovery probe skipped: Wi-Fi offline.");
    return false;
  }

  String response;
  int statusCode;

  if (!getConfigWithRetry(httpDeviceApi, response, statusCode, maxAttempts, timeoutMs))
  {
    if (response.length() > 0)
    {
      Serial.println(response);
    }

    if (registerApiFailure != nullptr)
    {
      registerApiFailure("config", statusCode);
    }

    return false;
  }

  JsonDocument doc;
  JsonObject config;
  String serverTimeUtc;
  String deviceType;
  int timezoneOffsetMinutes = 0;

  if (!configRuntime.parseConfigResponse(response, doc, config, serverTimeUtc, deviceType, timezoneOffsetMinutes))
  {
    if (registerApiFailure != nullptr)
    {
      registerApiFailure("config_parse", statusCode);
    }

    return false;
  }

  if (registerApiSuccess != nullptr)
  {
    registerApiSuccess("config_probe");
  }

  return true;
}

bool ConfigFlowRuntime::fetchConfig(
  bool wifiConnected,
  HttpDeviceApi &httpDeviceApi,
  ConfigRuntime &configRuntime,
  FountainConfig &fountainConfig,
  DeviceClock &deviceClock,
  FountainOutputState &outputs,
  FountainDailyTimeline &dailyTimeline,
  String &serverTimeUtc,
  String &deviceType,
  int maxAttempts,
  unsigned long timeoutMs,
  ConfigFlowApiSuccessCallback registerApiSuccess,
  ConfigFlowApiFailureCallback registerApiFailure,
  ConfigFlowMessageCallback markOutputStateTrusted,
  ConfigFlowVoidCallback applySafetyAndSyncHardware
)
{
  if (!wifiConnected)
  {
    Serial.println("Config fetch skipped: Wi-Fi offline.");
    return false;
  }

  String response;
  int statusCode;

  if (!getConfigWithRetry(httpDeviceApi, response, statusCode, maxAttempts, timeoutMs))
  {
    if (response.length() > 0)
    {
      Serial.println(response);
    }

    if (registerApiFailure != nullptr)
    {
      registerApiFailure("config", statusCode);
    }

    return false;
  }

  JsonDocument doc;
  JsonObject config;
  int timezoneOffsetMinutes = 0;

  if (!configRuntime.parseConfigResponse(response, doc, config, serverTimeUtc, deviceType, timezoneOffsetMinutes))
  {
    if (registerApiFailure != nullptr)
    {
      registerApiFailure("config_parse", statusCode);
    }

    return false;
  }

  if (registerApiSuccess != nullptr)
  {
    registerApiSuccess("config");
  }

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

  if (markOutputStateTrusted != nullptr)
  {
    markOutputStateTrusted("fresh Laravel config fetched");
  }

  if (applySafetyAndSyncHardware != nullptr)
  {
    applySafetyAndSyncHardware();
  }

  fountainConfig.loadDailyTimeline(config["daily_timeline"].as<JsonObject>(), dailyTimeline);
  fountainConfig.printDailyTimeline(dailyTimeline);

  int configJsonLength = 0;
  configRuntime.saveCompactConfigCache(fountainConfig, config, configJsonLength);

  Serial.print("Compact config cache JSON length: ");
  Serial.println(configJsonLength);

  return true;
}
