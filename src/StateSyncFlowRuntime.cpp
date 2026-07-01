#include "StateSyncFlowRuntime.h"

bool StateSyncFlowRuntime::postState(
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
)
{
  if (!wifiConnected)
  {
    Serial.println("State sync skipped: Wi-Fi offline.");
    return false;
  }

  if (apiServerOffline)
  {
    Serial.println("State sync skipped: API offline mode.");
    return false;
  }

  if (!outputStateTrusted)
  {
    Serial.println("State sync skipped: output state is not trusted yet. Safe boot OFF will not be pushed to Laravel.");
    return false;
  }

  if (updateWaterReadings != nullptr)
  {
    updateWaterReadings();
  }

  if (enforceWaterSafety != nullptr)
  {
    enforceWaterSafety();
  }

  String response;
  int statusCode;

  stateSyncRuntime.postState(
    httpDeviceApi,
    source,
    outputs,
    readings,
    firmwareVersion,
    response,
    statusCode
  );

  if (statusCode == 204 && response.length() == 0)
  {
    // 204 is used internally by StateSyncRuntime to mean no HTTP request was
    // needed because the payload is unchanged. Do not reset API health from a
    // skipped local optimization.
    return true;
  }

  Serial.print("State HTTP status: ");
  Serial.println(statusCode);

  if (statusCode < 200 || statusCode >= 300)
  {
    Serial.println(response);

    if (registerApiFailure != nullptr)
    {
      registerApiFailure("state", statusCode);
    }

    return false;
  }

  if (registerApiSuccess != nullptr)
  {
    registerApiSuccess("state");
  }

  Serial.println("State synced.");
  return true;
}

bool StateSyncFlowRuntime::postPendingOutputState(
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
)
{
  return postState(
    wifiConnected,
    apiServerOffline,
    outputStateTrusted,
    httpDeviceApi,
    stateSyncRuntime,
    outputs,
    readings,
    firmwareVersion,
    stateSyncRuntime.pendingOutputSource(),
    updateWaterReadings,
    enforceWaterSafety,
    registerApiSuccess,
    registerApiFailure
  );
}

bool StateSyncFlowRuntime::syncLocalStateIfDue(
  unsigned long now,
  unsigned long retryMs,
  bool wifiConnected,
  bool apiServerOffline,
  StateSyncRuntime &stateSyncRuntime,
  StateSyncPostCallback postPendingOutputState,
  unsigned long &lastStateSyncAt
)
{
  bool synced = stateSyncRuntime.syncLocalStateIfDue(
    now,
    retryMs,
    wifiConnected,
    apiServerOffline,
    postPendingOutputState
  );

  if (synced)
  {
    lastStateSyncAt = now;
  }

  return synced;
}