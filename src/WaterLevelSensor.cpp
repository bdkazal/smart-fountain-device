#include "WaterLevelSensor.h"

#if __has_include("DevFlags.h")
#include "DevFlags.h"
#endif

#if __has_include("HardwarePins.h")
#include "HardwarePins.h"
#else
#include "HardwarePins.example.h"
#endif

#ifndef SIMULATE_WATER_LOW
#define SIMULATE_WATER_LOW false
#endif

#ifndef WATER_LEVEL_SWITCH_PIN
#define WATER_LEVEL_SWITCH_PIN -1
#endif

#ifndef WATER_LEVEL_SWITCH_ACTIVE_LOW
#define WATER_LEVEL_SWITCH_ACTIVE_LOW 1
#endif

#ifndef WATER_LEVEL_SWITCH_USE_PULLUP
#define WATER_LEVEL_SWITCH_USE_PULLUP 1
#endif

#ifndef WATER_LEVEL_SWITCH_DEBOUNCE_MS
#define WATER_LEVEL_SWITCH_DEBOUNCE_MS 150
#endif

void WaterLevelSensor::begin()
{
  switchConfigured = WATER_LEVEL_SWITCH_PIN >= 0;

  if (!switchConfigured)
  {
    Serial.println("WaterLevelSensor using simulation mode.");
    return;
  }

  if (WATER_LEVEL_SWITCH_USE_PULLUP == 1)
  {
    pinMode(WATER_LEVEL_SWITCH_PIN, INPUT_PULLUP);
  }
  else
  {
    pinMode(WATER_LEVEL_SWITCH_PIN, INPUT);
  }

  lastRawSwitchWaterLow = readSwitchWaterLow();
  debouncedWaterLow = lastRawSwitchWaterLow;
  lastRawSwitchChangedAt = millis();

  Serial.print("WaterLevelSensor switch ready: GPIO ");
  Serial.print(WATER_LEVEL_SWITCH_PIN);
  Serial.print(" active_");
  Serial.print(WATER_LEVEL_SWITCH_ACTIVE_LOW == 1 ? "LOW" : "HIGH");
  Serial.print(" debounce_ms=");
  Serial.println(WATER_LEVEL_SWITCH_DEBOUNCE_MS);
}

void WaterLevelSensor::update(FountainReadings &readings)
{
  if (switchConfigured)
  {
    applyWaterLowState(readings, debouncedSwitchWaterLow(), "switch");
    return;
  }

  applyWaterLowState(readings, SIMULATE_WATER_LOW, "simulation");
}

bool WaterLevelSensor::readSwitchWaterLow() const
{
  int pinState = digitalRead(WATER_LEVEL_SWITCH_PIN);

  if (WATER_LEVEL_SWITCH_ACTIVE_LOW == 1)
  {
    return pinState == LOW;
  }

  return pinState == HIGH;
}

bool WaterLevelSensor::debouncedSwitchWaterLow()
{
  bool rawWaterLow = readSwitchWaterLow();
  unsigned long now = millis();

  if (rawWaterLow != lastRawSwitchWaterLow)
  {
    lastRawSwitchWaterLow = rawWaterLow;
    lastRawSwitchChangedAt = now;
  }

  if (debouncedWaterLow != rawWaterLow && now - lastRawSwitchChangedAt >= WATER_LEVEL_SWITCH_DEBOUNCE_MS)
  {
    debouncedWaterLow = rawWaterLow;
  }

  return debouncedWaterLow;
}

void WaterLevelSensor::applyWaterLowState(FountainReadings &readings, bool waterLow, const char *source)
{
  if (!hasLoggedInitialState || lastWaterLow != waterLow)
  {
    Serial.print("Water-low ");
    Serial.print(source);
    Serial.print(" changed: ");
    Serial.println(waterLow ? "LOW WATER" : "WATER OK");

    hasLoggedInitialState = true;
    lastWaterLow = waterLow;
  }

  readings.waterLow = waterLow;

  if (readings.waterLow)
  {
    readings.waterLevelPercent = 0;
    readings.waterLevelRaw = 0;
  }
  else
  {
    readings.waterLevelPercent = 50;
    readings.waterLevelRaw = 2048;
  }
}
