#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "HttpDeviceApi.h"
#include "FountainOutputs.h"

class CommandRuntime
{
public:
  bool ackCommand(
    HttpDeviceApi &httpDeviceApi,
    int commandId,
    const char *status,
    const char *message,
    String &response,
    int &statusCode
  );

  bool fetchCommand(
    HttpDeviceApi &httpDeviceApi,
    JsonDocument &doc,
    String &response,
    int &statusCode,
    bool &parseOk
  );

  bool handleOutputSet(
    JsonObject payload,
    FountainOutputs &fountainOutputs,
    FountainOutputState &outputs,
    FountainReadings &readings
  );

  bool handleSceneApply(
    JsonObject payload,
    FountainOutputs &fountainOutputs,
    FountainOutputState &outputs,
    FountainReadings &readings
  );
};
