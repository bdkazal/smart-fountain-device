#include "WifiReset.h"

#include <Arduino.h>

#include "DeviceStorage.h"
#include "WifiConfig.h"

namespace
{
static const int WIFI_RESET_BUTTON_PIN = 33;

bool buttonHolding = false;
bool resetTriggeredDuringHold = false;
unsigned long buttonHoldStartedAt = 0;

bool isButtonPressed()
{
  return digitalRead(WIFI_RESET_BUTTON_PIN) == LOW;
}

void requestWifiReset()
{
  clearStoredWifiCredentials();
  requestWifiSetupPortalOnNextBoot();

  Serial.println("Wi-Fi reset confirmed.");
  Serial.println("Setup hotspot will start now and remain required until valid Wi-Fi is saved.");
}
}

void beginWifiResetButton()
{
  pinMode(WIFI_RESET_BUTTON_PIN, INPUT_PULLUP);

  Serial.println();
  Serial.println("Wi-Fi reset button initialized.");
  Serial.print("Wi-Fi reset GPIO: ");
  Serial.println(WIFI_RESET_BUTTON_PIN);
  Serial.println("Hold Wi-Fi reset button to GND for 3 seconds to clear saved Wi-Fi and start setup hotspot.");
}

bool checkWifiResetOnBoot()
{
  beginWifiResetButton();

  Serial.println();
  Serial.println("Checking Wi-Fi reset button on boot...");

  if (!isButtonPressed())
  {
    Serial.println("Wi-Fi reset button not pressed.");
    return false;
  }

  Serial.println("Wi-Fi reset button pressed. Keep holding for 3 seconds...");
  const unsigned long startedAt = millis();

  while (millis() - startedAt < WifiConfig::ResetHoldMs)
  {
    if (!isButtonPressed())
    {
      Serial.println("Wi-Fi reset cancelled: button released too early.");
      return false;
    }

    delay(10);
  }

  requestWifiReset();
  resetTriggeredDuringHold = true;
  buttonHolding = true;
  buttonHoldStartedAt = startedAt;
  return true;
}

bool updateWifiResetButton()
{
  const bool pressed = isButtonPressed();

  if (!pressed)
  {
    buttonHolding = false;
    resetTriggeredDuringHold = false;
    buttonHoldStartedAt = 0;
    return false;
  }

  const unsigned long now = millis();

  if (!buttonHolding)
  {
    buttonHolding = true;
    resetTriggeredDuringHold = false;
    buttonHoldStartedAt = now;
    Serial.println("Wi-Fi reset button pressed. Hold for 3 seconds to start setup hotspot.");
    return false;
  }

  if (resetTriggeredDuringHold)
  {
    return false;
  }

  if (now - buttonHoldStartedAt < WifiConfig::ResetHoldMs)
  {
    return false;
  }

  requestWifiReset();
  resetTriggeredDuringHold = true;
  return true;
}
