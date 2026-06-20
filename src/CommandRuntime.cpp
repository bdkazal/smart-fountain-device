#include "CommandRuntime.h"

#include <ArduinoJson.h>

#include "DeviceSecrets.h"

bool CommandRuntime::ackCommand(
  HttpDeviceApi &httpDeviceApi,
  int commandId,
  const char *status,
  const char *message,
  String &response,
  int &statusCode
)
{
  JsonDocument doc;
  doc["device_uuid"] = DEVICE_UUID;
  doc["status"] = status;

  if (message != nullptr)
  {
    doc["message"] = message;
  }

  String payload;
  serializeJson(doc, payload);

  Serial.print("ACK command ");
  Serial.print(commandId);
  Serial.print(" as ");
  Serial.println(status);

  bool ok = httpDeviceApi.ackCommand(commandId, payload, response, statusCode);

  Serial.print("ACK HTTP status: ");
  Serial.println(statusCode);

  return ok;
}

bool CommandRuntime::fetchCommand(
  HttpDeviceApi &httpDeviceApi,
  JsonDocument &doc,
  String &response,
  int &statusCode,
  bool &parseOk
)
{
  response = "";
  statusCode = -1;
  parseOk = false;

  bool ok = httpDeviceApi.getCommands(response, statusCode);

  if (!ok)
  {
    Serial.println();
    Serial.print("GET ");
    Serial.println(httpDeviceApi.commandsUrl());
    Serial.print("Command HTTP status: ");
    Serial.println(statusCode);
    Serial.println(response);
    return false;
  }

  DeserializationError error = deserializeJson(doc, response);

  if (error)
  {
    Serial.print("Command JSON parse failed: ");
    Serial.println(error.c_str());
    parseOk = false;
    return false;
  }

  parseOk = true;
  return true;
}

bool CommandRuntime::handleOutputSet(
  JsonObject payload,
  FountainOutputs &fountainOutputs,
  FountainOutputState &outputs,
  FountainReadings &readings
)
{
  const char *outputKey = payload["output"] | "";
  JsonObject state = payload["state"].as<JsonObject>();
  const char *source = payload["source"] | "dashboard";

  if (strlen(outputKey) == 0 || state.isNull())
  {
    Serial.println("Invalid output_set payload.");
    return false;
  }

  bool applied = fountainOutputs.applyOutput(outputKey, state, source, outputs, readings);
  return applied;
}

bool CommandRuntime::handleSceneApply(
  JsonObject payload,
  FountainOutputs &fountainOutputs,
  FountainOutputState &outputs,
  FountainReadings &readings
)
{
  JsonObject sceneOutputs = payload["outputs"].as<JsonObject>();
  const char *source = payload["source"] | "scene_apply";

  if (sceneOutputs.isNull())
  {
    Serial.println("Invalid scene_apply payload: outputs missing.");
    return false;
  }

  bool allApplied = true;

  for (JsonPair outputPair : sceneOutputs)
  {
    const char *outputKey = outputPair.key().c_str();
    JsonObject state = outputPair.value().as<JsonObject>();

    if (!fountainOutputs.applyOutput(outputKey, state, source, outputs, readings))
    {
      allApplied = false;
    }
  }

  return allApplied;
}
