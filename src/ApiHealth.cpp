#include "ApiHealth.h"

void ApiHealth::begin(int offlineFailureThreshold, unsigned long probeIntervalMs)
{
  failureThreshold = offlineFailureThreshold > 0 ? offlineFailureThreshold : 3;
  probeInterval = probeIntervalMs > 0 ? probeIntervalMs : 30000;
  lastProbeAt = 0;
  failureCount = 0;
  serverOffline = false;
}

void ApiHealth::registerSuccess(const char *requestName)
{
  if (serverOffline)
  {
    Serial.print("API recovered. request=");
    Serial.println(requestName);
  }
  else if (failureCount > 0)
  {
    Serial.print("API recovered after warning(s). count=");
    Serial.print(failureCount);
    Serial.print(" request=");
    Serial.println(requestName);
  }

  failureCount = 0;
  serverOffline = false;
  lastProbeAt = 0;
}

void ApiHealth::registerFailure(const char *requestName, int statusCode)
{
  failureCount++;

  if (failureCount < failureThreshold)
  {
    Serial.print("API warning #");
    Serial.print(failureCount);
    Serial.print(" request=");
    Serial.print(requestName);
    Serial.print(" status=");
    Serial.println(statusCode);
    return;
  }

  if (!serverOffline)
  {
    serverOffline = true;
    lastProbeAt = 0;

    Serial.print("API server-offline mode entered after ");
    Serial.print(failureCount);
    Serial.print(" repeated failure(s). request=");
    Serial.print(requestName);
    Serial.print(" status=");
    Serial.println(statusCode);
    return;
  }

  Serial.print("API offline probe failed. request=");
  Serial.print(requestName);
  Serial.print(" status=");
  Serial.println(statusCode);
}

void ApiHealth::registerWarning(const char *requestName, int statusCode)
{
  Serial.print("API warning request=");
  Serial.print(requestName);
  Serial.print(" status=");
  Serial.print(statusCode);
  Serial.println(" not counted for server-offline mode.");
}

bool ApiHealth::isServerOffline() const
{
  return serverOffline;
}

bool ApiHealth::shouldProbe(unsigned long now) const
{
  if (!serverOffline)
  {
    return false;
  }

  if (lastProbeAt == 0)
  {
    return true;
  }

  return now - lastProbeAt >= probeInterval;
}

void ApiHealth::markProbeAttempt(unsigned long now)
{
  lastProbeAt = now;
}

int ApiHealth::consecutiveFailures() const
{
  return failureCount;
}
