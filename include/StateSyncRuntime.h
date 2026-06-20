#pragma once

#include <Arduino.h>
#include "FountainTypes.h"
#include "HttpDeviceApi.h"

typedef bool (*StateSyncPostCallback)();

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

  void queueLocalStateSync(const char *source = "local_button");
  bool hasPendingLocalSync() const;
  bool shouldSyncLocalState(unsigned long now, unsigned long retryMs) const;
  bool syncLocalStateIfDue(
    unsigned long now,
    unsigned long retryMs,
    bool wifiConnected,
    bool apiServerOffline,
    StateSyncPostCallback postLocalState
  );
  void markLocalStateSynced();
  void markLocalStateSyncFailed(unsigned long now);

  void rememberOutputSource(const char *source);
  const char *pendingOutputSource() const;
  const char *currentOutputSource() const;

private:
  const char *normalizeSource(const char *source) const;

  bool localSyncPending = false;
  unsigned long localSyncRetryAt = 0;

  const char *pendingSource = "local_button";
  const char *currentSource = "device_state";
};
