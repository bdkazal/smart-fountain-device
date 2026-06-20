#include "HttpDeviceApi.h"

#include <HTTPClient.h>

#include "DeviceSecrets.h"

void HttpDeviceApi::begin(ApiClient *client, unsigned long normalTimeoutMs, unsigned long commandTimeoutMs)
{
  apiClient = client;
  httpTimeoutMs = normalTimeoutMs;
  commandHttpTimeoutMs = commandTimeoutMs;
}

String HttpDeviceApi::configUrl() const
{
  if (apiClient == nullptr)
  {
    return "";
  }

  return apiClient->url("/api/device/config?device_uuid=" + String(DEVICE_UUID));
}

String HttpDeviceApi::commandsUrl() const
{
  if (apiClient == nullptr)
  {
    return "";
  }

  return apiClient->url("/api/device/commands?device_uuid=" + String(DEVICE_UUID));
}

String HttpDeviceApi::stateUrl() const
{
  if (apiClient == nullptr)
  {
    return "";
  }

  return apiClient->url("/api/device/state");
}

String HttpDeviceApi::ackUrl(int commandId) const
{
  if (apiClient == nullptr)
  {
    return "";
  }

  return apiClient->url("/api/device/commands/" + String(commandId) + "/ack");
}

bool HttpDeviceApi::getConfig(String &response, int &statusCode)
{
  return getUrl(configUrl(), httpTimeoutMs, response, statusCode);
}

bool HttpDeviceApi::getCommands(String &response, int &statusCode)
{
  return getUrl(commandsUrl(), commandHttpTimeoutMs, response, statusCode);
}

bool HttpDeviceApi::postState(const String &payload, String &response, int &statusCode)
{
  return postUrl(stateUrl(), payload, httpTimeoutMs, response, statusCode);
}

bool HttpDeviceApi::ackCommand(int commandId, const String &payload, String &response, int &statusCode)
{
  return postUrl(ackUrl(commandId), payload, commandHttpTimeoutMs, response, statusCode);
}

bool HttpDeviceApi::getUrl(const String &url, unsigned long timeoutMs, String &response, int &statusCode)
{
  response = "";
  statusCode = -1;

  if (apiClient == nullptr || url.length() == 0)
  {
    return false;
  }

  HTTPClient http;
  http.setTimeout(timeoutMs);
  http.begin(url);
  apiClient->addDeviceHeaders(http);

  statusCode = http.GET();
  response = http.getString();
  http.end();

  return statusCode >= 200 && statusCode < 300;
}

bool HttpDeviceApi::postUrl(const String &url, const String &payload, unsigned long timeoutMs, String &response, int &statusCode)
{
  response = "";
  statusCode = -1;

  if (apiClient == nullptr || url.length() == 0)
  {
    return false;
  }

  HTTPClient http;
  http.setTimeout(timeoutMs);
  http.begin(url);
  apiClient->addDeviceHeaders(http);

  statusCode = http.POST(payload);
  response = http.getString();
  http.end();

  return statusCode >= 200 && statusCode < 300;
}
