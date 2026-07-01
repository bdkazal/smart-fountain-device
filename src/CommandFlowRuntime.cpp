#include "CommandFlowRuntime.h"

#include "DeviceSecrets.h"

#ifndef MQTT_ENABLED
#define MQTT_ENABLED false
#endif

#ifndef MQTT_CONNECTED_REST_COMMAND_POLL_INTERVAL_MS
#define MQTT_CONNECTED_REST_COMMAND_POLL_INTERVAL_MS 60000
#endif

bool CommandFlowRuntime::shouldRunRestCommandPoll(unsigned long now)
{
  if (!MQTT_ENABLED)
  {
    return true;
  }

  if (lastRestCommandPollAt != 0 && now - lastRestCommandPollAt < MQTT_CONNECTED_REST_COMMAND_POLL_INTERVAL_MS)
  {
    return false;
  }

  lastRestCommandPollAt = now;
  Serial.println("REST command fallback poll while MQTT command transport is enabled.");

  return true;
}

bool CommandFlowRuntime::wasRecentlyProcessed(int commandId) const
{
  if (commandId <= 0)
  {
    return false;
  }

  for (int i = 0; i < PROCESSED_COMMAND_HISTORY_SIZE; i++)
  {
    if (processedCommandIds[i] == commandId)
    {
      return true;
    }
  }

  return false;
}

void CommandFlowRuntime::rememberProcessedCommand(int commandId)
{
  if (commandId <= 0 || wasRecentlyProcessed(commandId))
  {
    return;
  }

  processedCommandIds[processedCommandHistoryIndex] = commandId;
  processedCommandHistoryIndex = (processedCommandHistoryIndex + 1) % PROCESSED_COMMAND_HISTORY_SIZE;
}

bool CommandFlowRuntime::pollAndProcess(
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
)
{
  if (!wifiConnected)
  {
    Serial.println("Command poll skipped: Wi-Fi offline.");
    return false;
  }

  if (apiServerOffline)
  {
    return false;
  }

  if (!shouldRunRestCommandPoll(now))
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
    if (registerApiFailure != nullptr)
    {
      registerApiFailure("commands", statusCode);
    }

    return false;
  }

  if (registerApiSuccess != nullptr)
  {
    registerApiSuccess("commands");
  }

  if (!fetched || !parseOk)
  {
    if (registerApiFailure != nullptr)
    {
      registerApiFailure("commands_parse", statusCode);
    }

    return false;
  }

  JsonObject command = doc["command"].as<JsonObject>();

  if (command.isNull())
  {
    if (now - lastNoCommandLogAt >= noCommandLogIntervalMs)
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

  processCommand(
    command,
    wifiConnected,
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

  return true;
}

bool CommandFlowRuntime::ackCommand(
  bool wifiConnected,
  HttpDeviceApi &httpDeviceApi,
  CommandRuntime &commandRuntime,
  int commandId,
  const char *status,
  const char *message,
  CommandFlowApiSuccessCallback registerApiSuccess,
  CommandFlowApiFailureCallback registerApiFailure
)
{
  if (!wifiConnected)
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

    if (registerApiFailure != nullptr)
    {
      registerApiFailure("ack", statusCode);
    }

    return false;
  }

  if (registerApiSuccess != nullptr)
  {
    registerApiSuccess("ack");
  }

  return true;
}

bool CommandFlowRuntime::handleOutputSet(
  JsonObject payload,
  CommandRuntime &commandRuntime,
  FountainOutputs &fountainOutputs,
  FountainOutputState &outputs,
  FountainReadings &readings,
  CommandFlowVoidCallback updateWaterReadings,
  CommandFlowVoidCallback applySafetyAndSyncHardware
)
{
  if (updateWaterReadings != nullptr)
  {
    updateWaterReadings();
  }

  bool applied = commandRuntime.handleOutputSet(
    payload,
    fountainOutputs,
    outputs,
    readings
  );

  if (applySafetyAndSyncHardware != nullptr)
  {
    applySafetyAndSyncHardware();
  }

  return applied;
}

bool CommandFlowRuntime::handleSceneApply(
  JsonObject payload,
  CommandRuntime &commandRuntime,
  FountainOutputs &fountainOutputs,
  FountainOutputState &outputs,
  FountainReadings &readings,
  CommandFlowVoidCallback updateWaterReadings,
  CommandFlowVoidCallback applySafetyAndSyncHardware
)
{
  if (updateWaterReadings != nullptr)
  {
    updateWaterReadings();
  }

  bool allApplied = commandRuntime.handleSceneApply(
    payload,
    fountainOutputs,
    outputs,
    readings
  );

  if (applySafetyAndSyncHardware != nullptr)
  {
    applySafetyAndSyncHardware();
  }

  return allApplied;
}

void CommandFlowRuntime::processCommand(
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
)
{
  int commandId = command["id"] | 0;
  const char *commandType = command["command_type"] | "";
  JsonObject payload = command["payload"].as<JsonObject>();

  if (commandId <= 0 || strlen(commandType) == 0)
  {
    Serial.println("Invalid command shape. Ignoring.");
    return;
  }

  if (wasRecentlyProcessed(commandId))
  {
    Serial.print("Duplicate command ignored. command_id=");
    Serial.println(commandId);
    return;
  }

  rememberProcessedCommand(commandId);

  Serial.println();
  Serial.print("Processing command #");
  Serial.print(commandId);
  Serial.print(" type=");
  Serial.println(commandType);

  bool applied = false;
  String type = commandType;

  if (type == "output_set")
  {
    applied = handleOutputSet(
      payload,
      commandRuntime,
      fountainOutputs,
      outputs,
      readings,
      updateWaterReadings,
      applySafetyAndSyncHardware
    );
  }
  else if (type == "scene_apply")
  {
    applied = handleSceneApply(
      payload,
      commandRuntime,
      fountainOutputs,
      outputs,
      readings,
      updateWaterReadings,
      applySafetyAndSyncHardware
    );
  }
  else
  {
    ackCommand(
      wifiConnected,
      httpDeviceApi,
      commandRuntime,
      commandId,
      "failed",
      "Unsupported command type.",
      registerApiSuccess,
      registerApiFailure
    );
    return;
  }

  if (applied)
  {
    dailyTimelineRuntime.markCurrentRangeSatisfied(
      dailyTimeline,
      deviceClock,
      "cloud command applied"
    );

    if (markOutputStateTrusted != nullptr)
    {
      markOutputStateTrusted("cloud command applied");
    }

    stateSyncRuntime.rememberOutputSource("device_state");
  }

  ackCommand(
    wifiConnected,
    httpDeviceApi,
    commandRuntime,
    commandId,
    "acknowledged",
    nullptr,
    registerApiSuccess,
    registerApiFailure
  );

  if (applied)
  {
    ackCommand(
      wifiConnected,
      httpDeviceApi,
      commandRuntime,
      commandId,
      "executed",
      nullptr,
      registerApiSuccess,
      registerApiFailure
    );
  }
  else
  {
    ackCommand(
      wifiConnected,
      httpDeviceApi,
      commandRuntime,
      commandId,
      "failed",
      "Command could not be applied safely.",
      registerApiSuccess,
      registerApiFailure
    );
  }

  if (postState != nullptr)
  {
    postState("device_state");
  }
}
