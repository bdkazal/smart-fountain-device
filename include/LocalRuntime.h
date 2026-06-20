#pragma once

#include "FountainTypes.h"
#include "LocalControls.h"

class LocalRuntime
{
public:
  bool processControls(
    LocalControls &localControls,
    FountainOutputState &outputs,
    FountainReadings &readings
  );
};
