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
