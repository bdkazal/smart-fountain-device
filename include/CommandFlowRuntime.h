#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#include "CommandRuntime.h"
#include "DailyTimelineRuntime.h"
#include "DeviceClock.h"
#include "FountainOutputs.h"
#include "FountainTypes.h"
#include "HttpDeviceApi.h"
#include "StateSyncRuntime.h"

typedef void (*CommandFlowVoidCallback)();
typedef void (*CommandFlowApiSuccessCallback)(const char *requestName);
typedef void (*CommandFlowApiFailureCallback)(const char *requestName, int statusCode);
typedef void (*CommandFlowTrustCallback)(const char *reason);
typedef bool (*CommandFlowPostStateCallback)(const char *source);

class CommandFlowRuntime
{
public:
  bool pollAndProcess(
    unsigned long now,
    unsigned long noCommandLogIntervalMs,
    bool wifiConnected,
    bool apiServerOffline,
    HttpDeviceApi &httpDeviceApi,
    CommandRuntime &commandRuntime,
    FountainOutputs &fountainOutputs,
    DailyTimelineRuntime &dailyTimelineRuntime,
    DeviceClock &deviceClock,
    FountainDailyTimeline &dailyTimeline,
    FountainOutputState &outputs,
    FountainReadings &readings,
    StateSyncRuntime &stateSyncRuntime,
    CommandFlowApiSuccessCallback registerApiSuccess,
    CommandFlowApiFailureCallback registerApiFailure,
    CommandFlowVoidCallback updateWaterReadings,
    CommandFlowVoidCallback applySafetyAndSyncHardware,
    CommandFlowTrustCallback markOutputStateTrusted,
    CommandFlowPostStateCallback postState
  );

  void processCommand(
    JsonObject command,
    bool wifiConnected,
    HttpDeviceApi &httpDeviceApi,
    CommandRuntime &commandRuntime,
    FountainOutputs &fountainOutputs,
    DailyTimelineRuntime &dailyTimelineRuntime,
    DeviceClock &deviceClock,
    FountainDailyTimeline &dailyTimeline,
    FountainOutputState &outputs,
    FountainReadings &readings,
    StateSyncRuntime &stateSyncRuntime,
    CommandFlowApiSuccessCallback registerApiSuccess,
    CommandFlowApiFailureCallback registerApiFailure,
    CommandFlowVoidCallback updateWaterReadings,
    CommandFlowVoidCallback applySafetyAndSyncHardware,
    CommandFlowTrustCallback markOutputStateTrusted,
    CommandFlowPostStateCallback postState
  );

private:
  unsigned long lastNoCommandLogAt = 0;

  bool ackCommand(
    bool wifiConnected,
    HttpDeviceApi &httpDeviceApi,
    CommandRuntime &commandRuntime,
    int commandId,
    const char *status,
    const char *message,
    CommandFlowApiSuccessCallback registerApiSuccess,
    CommandFlowApiFailureCallback registerApiFailure
  );

  bool handleOutputSet(
    JsonObject payload,
    CommandRuntime &commandRuntime,
    FountainOutputs &fountainOutputs,
    FountainOutputState &outputs,
    FountainReadings &readings,
    CommandFlowVoidCallback updateWaterReadings,
    CommandFlowVoidCallback applySafetyAndSyncHardware
  );

  bool handleSceneApply(
    JsonObject payload,
    CommandRuntime &commandRuntime,
    FountainOutputs &fountainOutputs,
    FountainOutputState &outputs,
    FountainReadings &readings,
    CommandFlowVoidCallback updateWaterReadings,
    CommandFlowVoidCallback applySafetyAndSyncHardware
  );
};
