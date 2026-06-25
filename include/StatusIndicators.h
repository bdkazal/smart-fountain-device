#pragma once

#include <Arduino.h>

class StatusIndicators
{
public:
  void begin();

  void update(
    bool wifiConnected,
    bool serverOnline,
    bool waterLow,
    bool setupPortalActive
  );

private:
  bool networkLedConfigured = false;
  bool waterLedConfigured = false;
  bool networkLedState = false;
  unsigned long lastNetworkLedToggleAt = 0;

  void updateNetworkLed(
    bool wifiConnected,
    bool serverOnline,
    bool setupPortalActive,
    unsigned long now
  );

  void updateWaterSafetyLed(bool waterLow);
  void writeNetworkLed(bool on);
  void writeWaterSafetyLed(bool on);
  void writeLed(int pin, bool activeHigh, bool on);
};
