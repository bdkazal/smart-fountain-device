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
