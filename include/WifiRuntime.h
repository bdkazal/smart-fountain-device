#pragma once

#include <Arduino.h>

typedef void (*WifiRuntimeTickCallback)();

class WifiRuntime
{
public:
  bool isConnected() const;

  bool connectWithCredentials(
    const String &ssid,
    const String &password,
    WifiRuntimeTickCallback onWaitTick = nullptr
  );

  bool connect(
    bool allowDevelopmentFallback,
    WifiRuntimeTickCallback onWaitTick = nullptr
  );
};
