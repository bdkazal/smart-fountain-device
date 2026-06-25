#include "StatusIndicators.h"

#if __has_include("HardwarePins.h")
#include "HardwarePins.h"
#else
#include "HardwarePins.example.h"
#endif

#ifndef NET_STATUS_LED_PIN
#define NET_STATUS_LED_PIN -1
#endif

#ifndef NET_STATUS_LED_ACTIVE_HIGH
#define NET_STATUS_LED_ACTIVE_HIGH 1
#endif

#ifndef WATER_SAFETY_LED_PIN
#define WATER_SAFETY_LED_PIN -1
#endif

#ifndef WATER_SAFETY_LED_ACTIVE_HIGH
#define WATER_SAFETY_LED_ACTIVE_HIGH 1
#endif

#ifndef NET_STATUS_FAST_BLINK_MS
#define NET_STATUS_FAST_BLINK_MS 250
#endif

#ifndef NET_STATUS_SLOW_BLINK_MS
#define NET_STATUS_SLOW_BLINK_MS 1000
#endif

void StatusIndicators::begin()
{
  networkLedConfigured = NET_STATUS_LED_PIN >= 0;
  waterLedConfigured = WATER_SAFETY_LED_PIN >= 0;

  if (networkLedConfigured)
  {
    pinMode(NET_STATUS_LED_PIN, OUTPUT);
    writeNetworkLed(false);

    Serial.print("Network/server status LED ready: GPIO ");
    Serial.print(NET_STATUS_LED_PIN);
    Serial.print(" active_");
    Serial.println(NET_STATUS_LED_ACTIVE_HIGH == 1 ? "HIGH" : "LOW");
  }
  else
  {
    Serial.println("Network/server status LED disabled.");
  }

  if (waterLedConfigured)
  {
    pinMode(WATER_SAFETY_LED_PIN, OUTPUT);
    writeWaterSafetyLed(false);

    Serial.print("Water safety status LED ready: GPIO ");
    Serial.print(WATER_SAFETY_LED_PIN);
    Serial.print(" active_");
    Serial.println(WATER_SAFETY_LED_ACTIVE_HIGH == 1 ? "HIGH" : "LOW");
  }
  else
  {
    Serial.println("Water safety status LED disabled.");
  }
}

void StatusIndicators::update(
  bool wifiConnected,
  bool serverOnline,
  bool waterLow,
  bool setupPortalActive
)
{
  unsigned long now = millis();

  updateNetworkLed(wifiConnected, serverOnline, setupPortalActive, now);
  updateWaterSafetyLed(waterLow);
}

void StatusIndicators::updateNetworkLed(
  bool wifiConnected,
  bool serverOnline,
  bool setupPortalActive,
  unsigned long now
)
{
  if (!networkLedConfigured)
  {
    return;
  }

  if (wifiConnected && serverOnline && !setupPortalActive)
  {
    networkLedState = true;
    writeNetworkLed(true);
    return;
  }

  unsigned long blinkInterval = (!wifiConnected || setupPortalActive)
                                ? NET_STATUS_FAST_BLINK_MS
                                : NET_STATUS_SLOW_BLINK_MS;

  if (now - lastNetworkLedToggleAt >= blinkInterval)
  {
    networkLedState = !networkLedState;
    writeNetworkLed(networkLedState);
    lastNetworkLedToggleAt = now;
  }
}

void StatusIndicators::updateWaterSafetyLed(bool waterLow)
{
  if (!waterLedConfigured)
  {
    return;
  }

  writeWaterSafetyLed(waterLow);
}

void StatusIndicators::writeNetworkLed(bool on)
{
  writeLed(NET_STATUS_LED_PIN, NET_STATUS_LED_ACTIVE_HIGH == 1, on);
}

void StatusIndicators::writeWaterSafetyLed(bool on)
{
  writeLed(WATER_SAFETY_LED_PIN, WATER_SAFETY_LED_ACTIVE_HIGH == 1, on);
}

void StatusIndicators::writeLed(int pin, bool activeHigh, bool on)
{
  digitalWrite(pin, activeHigh ? (on ? HIGH : LOW) : (on ? LOW : HIGH));
}
