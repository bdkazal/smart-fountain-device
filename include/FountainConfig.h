#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "FountainTypes.h"

class FountainConfig
{
public:
  // Loads the latest platform output state from Laravel config into local state.
  // This prevents reboot from blindly forcing all outputs OFF before state sync.
  void loadInitialOutputs(JsonObject config, FountainOutputState &outputs);

  // Prints the daily timeline for development visibility. Later this same data
  // will be cached and used by the offline timeline runner.
  void printDailyTimeline(JsonObject dailyTimeline);

  String timelineRepeat() const;

private:
  String lastTimelineRepeat = "";

  // Laravel config may return an output as either { state: {...} } or directly
  // as a state-like object while the API contract is evolving.
  JsonObject outputStateObject(JsonObject outputConfig);
};
