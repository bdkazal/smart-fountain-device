#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#include "ConfigRuntime.h"
#include "DeviceClock.h"
#include "FountainConfig.h"
#include "FountainTypes.h"
#include "HttpDeviceApi.h"

typedef void (*ConfigFlowVoidCallback)();
typedef void (*ConfigFlowMessageCallback)(const char *reason);
typedef void (*ConfigFlowApiSuccessCallback)(const char *requestName);
typedef void (*ConfigFlowApiFailureCallback)(const char *requestName, int statusCode);

class ConfigFlowRuntime
{
public:
  bool loadCachedConfigIfAvailable(
    ConfigRuntime &configRuntime,
    FountainConfig &fountainConfig,
    FountainOutputState &outputs,
    FountainDailyTimeline &dailyTimeline,
    ConfigFlowMessageCallback markOutputStateTrusted,
    ConfigFlowMessageCallback markOutputStateUntrusted,
    ConfigFlowVoidCallback applySafetyAndSyncHardware
  );

  bool probeApiRecovery(
    bool wifiConnected,
    HttpDeviceApi &httpDeviceApi,
    ConfigRuntime &configRuntime,
    int maxAttempts,
    unsigned long timeoutMs,
    ConfigFlowApiSuccessCallback registerApiSuccess,
    ConfigFlowApiFailureCallback registerApiFailure
  );

  bool fetchConfig(
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
  );

private:
  bool getConfigWithRetry(
    HttpDeviceApi &httpDeviceApi,
    String &response,
    int &statusCode,
    int maxAttempts,
    unsigned long timeoutMs
  );
};
