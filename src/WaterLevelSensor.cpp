#include "WaterLevelSensor.h"
#include "DevFlags.h"

#ifndef SIMULATE_WATER_LOW
#define SIMULATE_WATER_LOW false
#endif

void WaterLevelSensor::begin()
{
  // Placeholder for real water-level sensor setup.
  // Later this will configure the ADC pin and calibration values.
}

void WaterLevelSensor::update(FountainReadings &readings)
{
  bool simulatedWaterLow = SIMULATE_WATER_LOW;

  if (readings.waterLow != simulatedWaterLow)
  {
    Serial.print("Water-low simulation changed: ");
    Serial.println(simulatedWaterLow ? "LOW WATER" : "WATER OK");
  }

  readings.waterLow = simulatedWaterLow;

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
