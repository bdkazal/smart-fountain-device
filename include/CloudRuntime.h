#pragma once

#include "ApiHealth.h"

class CloudRuntime
{
public:
  bool isOfflineControlMode(ApiHealth &apiHealth, bool wifiConnected) const;

  void registerSuccess(ApiHealth &apiHealth, const char *requestName);
  void registerFailure(ApiHealth &apiHealth, const char *requestName, int statusCode);

  void logModeIfChanged(ApiHealth &apiHealth, bool wifiConnected);

private:
  enum CloudControlMode
  {
    CLOUD_MODE_UNKNOWN,
    CLOUD_MODE_ONLINE,
    CLOUD_MODE_OFFLINE
  };

  CloudControlMode lastLoggedCloudMode = CLOUD_MODE_UNKNOWN;

  bool shouldCountApiFailureForOffline(const char *requestName) const;
};
