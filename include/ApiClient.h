#pragma once

#include <Arduino.h>
#include <HTTPClient.h>

class ApiClient
{
public:
  // Keeps Laravel API URL/header rules in one place so product code does not
  // repeat device-auth details across config, state, command, and ACK calls.
  void begin(const char *baseUrl, const char *deviceApiKey);

  // Builds a full API URL from a path like /api/device/config.
  // The base URL must be the Laravel LAN/server URL, not 127.0.0.1 from ESP32.
  String url(const String &path) const;

  // Adds the shared Biztola device headers to an open HTTPClient request.
  void addDeviceHeaders(HTTPClient &http) const;

private:
  String normalizedBaseUrl = "";
  String apiKey = "";
};
