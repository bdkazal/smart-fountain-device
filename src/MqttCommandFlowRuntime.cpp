#include "MqttCommandFlowRuntime.h"

#include "DeviceSecrets.h"

bool MqttCommandFlowRuntime::processPending(
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
)
{
  String topic;
  String message;

  if (!mqttCommandRuntime.takePendingCommand(topic, message))
  {
    return false;
  }

  Serial.println();
  Serial.print("MQTT command topic: ");
  Serial.println(topic);

  JsonDocument commandDoc;

  if (!parseMqttCommand(message, commandDoc))
  {
    return false;
  }

  JsonObject command = commandDoc.as<JsonObject>();

  commandFlowRuntime.processCommand(
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

bool MqttCommandFlowRuntime::parseMqttCommand(const String &message, JsonDocument &commandDoc)
{
  JsonDocument mqttDoc;
  DeserializationError error = deserializeJson(mqttDoc, message);

  if (error)
  {
    Serial.print("MQTT command JSON parse failed: ");
    Serial.println(error.c_str());
    return false;
  }

  JsonObject mqttCommand = mqttDoc.as<JsonObject>();
  const char *schema = mqttCommand["schema"] | "";

  if (String(schema) != "biztola.command.v1")
  {
    Serial.print("MQTT command ignored: unsupported schema=");
    Serial.println(schema);
    return false;
  }

  const char *deviceUuid = mqttCommand["device_uuid"] | "";

  if (String(deviceUuid) != DEVICE_UUID)
  {
    Serial.println("MQTT command ignored: device_uuid mismatch.");
    return false;
  }

  int commandId = mqttCommand["command_id"] | 0;

  if (commandId <= 0)
  {
    commandId = mqttCommand["id"] | 0;
  }

  const char *commandType = mqttCommand["command_type"] | "";
  JsonObject commandPayload = mqttCommand["payload"].as<JsonObject>();

  if (commandId <= 0 || String(commandType).length() == 0 || commandPayload.isNull())
  {
    Serial.println("MQTT command ignored: invalid command shape.");
    return false;
  }

  JsonObject command = commandDoc.to<JsonObject>();
  command["id"] = commandId;
  command["command_type"] = commandType;
  command["payload"].set(commandPayload);

  return true;
}
