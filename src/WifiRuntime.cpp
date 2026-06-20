#include "WifiRuntime.h"

#include <WiFi.h>

#include "DeviceSecrets.h"
#include "DeviceStorage.h"

bool WifiRuntime::isConnected() const
{
  return WiFi.status() == WL_CONNECTED;
}

bool WifiRuntime::connectWithCredentials(
  const String &ssid,
  const String &password,
  WifiRuntimeTickCallback onWaitTick
)
{
  if (ssid.length() == 0)
  {
    Serial.println("Cannot connect to Wi-Fi: SSID is empty.");
    return false;
  }

  Serial.println();
  Serial.println("Connecting Wi-Fi...");
  Serial.print("SSID: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.begin(ssid.c_str(), password.c_str());

  unsigned long startedAt = millis();

  while (!isConnected() && millis() - startedAt < 15000)
  {
    if (onWaitTick != nullptr)
    {
      onWaitTick();
    }

    delay(100);
    Serial.print(".");
  }

  Serial.println();

  if (isConnected())
  {
    Serial.print("Wi-Fi connected. IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI dBm: ");
    Serial.println(WiFi.RSSI());
    return true;
  }

  Serial.println("Wi-Fi connection failed.");
  return false;
}

bool WifiRuntime::connect(
  bool allowDevelopmentFallback,
  WifiRuntimeTickCallback onWaitTick
)
{
  StoredDeviceConfig storedConfig = loadStoredDeviceConfig();

  if (storedConfig.hasWifiCredentials)
  {
    Serial.println("Trying stored Wi-Fi credentials...");

    if (connectWithCredentials(storedConfig.wifiSsid, storedConfig.wifiPassword, onWaitTick))
    {
      return true;
    }

    Serial.println("Stored Wi-Fi failed.");
  }
  else
  {
    Serial.println("No stored Wi-Fi credentials found.");
  }

  if (allowDevelopmentFallback)
  {
    Serial.println("Trying DeviceSecrets Wi-Fi as development fallback...");

    if (connectWithCredentials(WIFI_SSID, WIFI_PASSWORD, onWaitTick))
    {
      return true;
    }

    Serial.println("DeviceSecrets Wi-Fi failed.");
  }

  Serial.println("Wi-Fi unavailable. Will retry later. Setup hotspot starts only after GPIO33 Wi-Fi reset.");
  return false;
}
