#pragma once

#include <Arduino.h>

class ApiHealth
{
public:
  void begin(int offlineFailureThreshold, unsigned long probeIntervalMs);

  void registerSuccess(const char *requestName);
  void registerFailure(const char *requestName, int statusCode);
  void registerWarning(const char *requestName, int statusCode);

  bool isServerOffline() const;
  bool shouldProbe(unsigned long now) const;
  void markProbeAttempt(unsigned long now);

  int consecutiveFailures() const;

private:
  int failureThreshold = 3;
  unsigned long probeInterval = 30000;
  unsigned long lastProbeAt = 0;
  int failureCount = 0;
  bool serverOffline = false;
};
