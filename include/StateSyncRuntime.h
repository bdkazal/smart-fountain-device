#pragma once

#include <Arduino.h>
#include "FountainTypes.h"

class StateSyncRuntime
{
public:
  String buildStatePayload(
    const char *source,
    const FountainOutputState &outputs,
    const FountainReadings &readings,
    const char *firmwareVersion
  );
};
