#pragma once

#include <Arduino.h>
#include "ApiClient.h"

class HttpDeviceApi
{
public:
  void begin(ApiClient *client, unsigned long normalTimeoutMs, unsigned long commandTimeoutMs);

  String configUrl() const;
  String commandsUrl() const;
  String stateUrl() const;
  String ackUrl(int commandId) const;

  bool getConfig(String &response, int &statusCode);
  bool getCommands(String &response, int &statusCode);
  bool postState(const String &payload, String &response, int &statusCode);
  bool ackCommand(int commandId, const String &payload, String &response, int &statusCode);

private:
  ApiClient *apiClient = nullptr;
  unsigned long httpTimeoutMs = 5000;
  unsigned long commandHttpTimeoutMs = 5000;

  bool getUrl(const String &url, unsigned long timeoutMs, String &response, int &statusCode);
  bool postUrl(const String &url, const String &payload, unsigned long timeoutMs, String &response, int &statusCode);
};
