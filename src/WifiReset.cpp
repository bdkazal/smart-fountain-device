#include "WifiReset.h"

#include <Arduino.h>

#include "ConfigCache.h"
#include "DeviceStorage.h"

static const int WIFI_RESET_BUTTON_PIN = 33;
static const unsigned long WIFI_RESET_HOLD_MS = 3000;

void checkWifiResetOnBoot()
{
  pinMode(WIFI_RESET_BUTTON_PIN, INPUT_PULLUP);

  Serial.println();
  Serial.println("Checking Wi-Fi reset button...");
  Serial.print("Wi-Fi reset GPIO: ");
  Serial.println(WIFI_RESET_BUTTON_PIN);
  Serial.println("Hold Wi-Fi reset button during startup to clear saved Wi-Fi and cached runtime config.");

  if (digitalRead(WIFI_RESET_BUTTON_PIN) == HIGH)
  {
    Serial.println("Wi-Fi reset button not pressed.");
    return;
  }

  Serial.println("Wi-Fi reset button pressed. Keep holding...");

  unsigned long startedAt = millis();

  while (millis() - startedAt < WIFI_RESET_HOLD_MS)
  {
    if (digitalRead(WIFI_RESET_BUTTON_PIN) == HIGH)
    {
      Serial.println("Wi-Fi reset cancelled. Button released too early.");
      delay(1000);
      return;
    }

    Serial.print(".");
    delay(250);
  }

  Serial.println();
  Serial.println("Wi-Fi reset confirmed.");

  clearStoredWifiCredentials();
  clearCachedConfigJson();
  requestWifiSetupPortalOnNextBoot();

  Serial.println("Saved Wi-Fi and cached runtime config cleared. Restarting into setup portal...");
  delay(1000);

  ESP.restart();
}
