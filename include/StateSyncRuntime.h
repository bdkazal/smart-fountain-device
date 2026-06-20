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

  void queueLocalStateSync();
  bool hasPendingLocalSync() const;
  bool shouldSyncLocalState(unsigned long now, unsigned long retryMs) const;
  void markLocalStateSynced();
  void markLocalStateSyncFailed(unsigned long now);

private:
  bool localSyncPending = false;
  unsigned long localSyncRetryAt = 0;
};
