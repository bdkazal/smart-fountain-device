#pragma once

#include <Arduino.h>
#include "FountainTypes.h"
#include "HttpDeviceApi.h"

class StateSyncRuntime
{
public:
  String buildStatePayload(
    const char *source,
    const FountainOutputState &outputs,
    const FountainReadings &readings,
    const char *firmwareVersion
  );

  bool postState(
    HttpDeviceApi &httpDeviceApi,
    const char *source,
    const FountainOutputState &outputs,
    const FountainReadings &readings,
    const char *firmwareVersion,
    String &response,
    int &statusCode
  );
};
