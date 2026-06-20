#include "CloudRuntime.h"

#include <Arduino.h>

bool CloudRuntime::isOfflineControlMode(ApiHealth &apiHealth, bool wifiConnected) const
{
  return !wifiConnected || apiHealth.isServerOffline();
}

void CloudRuntime::registerSuccess(ApiHealth &apiHealth, const char *requestName)
{
  apiHealth.registerSuccess(requestName);
}

void CloudRuntime::registerFailure(ApiHealth &apiHealth, const char *requestName, int statusCode)
{
  if (shouldCountApiFailureForOffline(requestName))
  {
    apiHealth.registerFailure(requestName, statusCode);
    return;
  }

  apiHealth.registerWarning(requestName, statusCode);
}

void CloudRuntime::logModeIfChanged(ApiHealth &apiHealth, bool wifiConnected)
{
  CloudControlMode currentMode = isOfflineControlMode(apiHealth, wifiConnected)
                                   ? CLOUD_MODE_OFFLINE
                                   : CLOUD_MODE_ONLINE;

  if (currentMode == lastLoggedCloudMode)
  {
    return;
  }

  lastLoggedCloudMode = currentMode;

  if (currentMode == CLOUD_MODE_ONLINE)
  {
    Serial.println("Cloud mode: ONLINE - Laravel reachable, dashboard/API control active.");
    return;
  }

  if (!wifiConnected)
  {
    Serial.println("Cloud mode: OFFLINE - Wi-Fi disconnected, local buttons/water safety active, keeping last trusted saved output state.");
  }
  else
  {
    Serial.println("Cloud mode: OFFLINE - API unavailable, local buttons/water safety active, keeping last trusted saved output state.");
  }
}

bool CloudRuntime::shouldCountApiFailureForOffline(const char *requestName) const
{
  String request = requestName;
  return request == "commands" || request == "state" || request == "config";
}
