#pragma once

#include <Arduino.h>
#include "HttpDeviceApi.h"

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
};
