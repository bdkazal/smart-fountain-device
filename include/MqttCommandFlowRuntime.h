#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#include "CommandFlowRuntime.h"
#include "CommandRuntime.h"
#include "DailyTimelineRuntime.h"
#include "DeviceClock.h"
#include "FountainOutputs.h"
#include "FountainTypes.h"
#include "HttpDeviceApi.h"
#include "MqttCommandRuntime.h"
#include "StateSyncRuntime.h"

class MqttCommandFlowRuntime
{
public:
  bool processPending(
    MqttCommandRuntime &mqttCommandRuntime,
    bool wifiConnected,
    HttpDeviceApi &httpDeviceApi,
    CommandRuntime &commandRuntime,
    CommandFlowRuntime &commandFlowRuntime,
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
  bool parseMqttCommand(const String &message, JsonDocument &commandDoc);
};
