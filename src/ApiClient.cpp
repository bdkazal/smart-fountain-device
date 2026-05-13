#include "ApiClient.h"

void ApiClient::begin(const char *baseUrl, const char *deviceApiKey)
{
  normalizedBaseUrl = String(baseUrl);
  apiKey = String(deviceApiKey);

  // Avoid double slashes when callers pass paths like /api/device/state.
  if (normalizedBaseUrl.endsWith("/"))
  {
    normalizedBaseUrl.remove(normalizedBaseUrl.length() - 1);
  }
}

String ApiClient::url(const String &path) const
{
  return normalizedBaseUrl + path;
}

void ApiClient::addDeviceHeaders(HTTPClient &http) const
{
  // All firmware-to-Laravel device requests use the same headers.
  // X-DEVICE-KEY authenticates the physical device; request body/query still
  // carries device_uuid so Laravel can bind the request to the correct row.
  http.addHeader("Accept", "application/json");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-DEVICE-KEY", apiKey);
}
