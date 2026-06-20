#pragma once

#include <Arduino.h>

#include "FountainTypes.h"
#include "HttpDeviceApi.h"
#include "StateSyncRuntime.h"

typedef void (*StateSyncFlowVoidCallback)();
typedef void (*StateSyncFlowApiSuccessCallback)(const char *requestName);
typedef void (*StateSyncFlowApiFailureCallback)(const char *requestName, int statusCode);

class StateSyncFlowRuntime
{
public:
  bool postState(
    bool wifiConnected,
    bool apiServerOffline,
    bool outputStateTrusted,
    HttpDeviceApi &httpDeviceApi,
    StateSyncRuntime &stateSyncRuntime,
    FountainOutputState &outputs,
    FountainReadings &readings,
    const char *firmwareVersion,
    const char *source,
    StateSyncFlowVoidCallback updateWaterReadings,
    StateSyncFlowVoidCallback enforceWaterSafety,
    StateSyncFlowApiSuccessCallback registerApiSuccess,
    StateSyncFlowApiFailureCallback registerApiFailure
  );

  bool postPendingOutputState(
    bool wifiConnected,
    bool apiServerOffline,
    bool outputStateTrusted,
    HttpDeviceApi &httpDeviceApi,
    StateSyncRuntime &stateSyncRuntime,
    FountainOutputState &outputs,
    FountainReadings &readings,
    const char *firmwareVersion,
    StateSyncFlowVoidCallback updateWaterReadings,
    StateSyncFlowVoidCallback enforceWaterSafety,
    StateSyncFlowApiSuccessCallback registerApiSuccess,
    StateSyncFlowApiFailureCallback registerApiFailure
  );

  bool syncLocalStateIfDue(
    unsigned long now,
    unsigned long retryMs,
    bool wifiConnected,
    bool apiServerOffline,
    StateSyncRuntime &stateSyncRuntime,
    StateSyncPostCallback postPendingOutputState,
    unsigned long &lastStateSyncAt
  );
};
