#include "CobOutputDriver.h"

#if __has_include("HardwarePins.h")
#include "HardwarePins.h"
#else
#include "HardwarePins.example.h"
#endif

#ifndef COB_OUTPUT_PIN
#define COB_OUTPUT_PIN -1
#endif

#ifndef COB_OUTPUT_ACTIVE_HIGH
#define COB_OUTPUT_ACTIVE_HIGH 1
#endif

void CobOutputDriver::begin()
{
  if (COB_OUTPUT_PIN < 0)
  {
    Serial.println("HardwareOutputs COB pin not configured.");
    return;
  }

  pinMode(COB_OUTPUT_PIN, OUTPUT);
  digitalWrite(COB_OUTPUT_PIN, COB_OUTPUT_ACTIVE_HIGH == 1 ? LOW : HIGH);
  Serial.print("HardwareOutputs COB pin ready: GPIO ");
  Serial.print(COB_OUTPUT_PIN);
  Serial.print(" active_");
  Serial.println(COB_OUTPUT_ACTIVE_HIGH == 1 ? "HIGH" : "LOW");
}

void CobOutputDriver::apply(const FountainOutputState &outputs)
{
  if (COB_OUTPUT_PIN < 0)
  {
    return;
  }

  bool cobOn = outputs.cobEnabled;
  bool activeHigh = COB_OUTPUT_ACTIVE_HIGH == 1;
  int onLevel = activeHigh ? HIGH : LOW;
  int offLevel = activeHigh ? LOW : HIGH;
  int outputLevel = cobOn ? onLevel : offLevel;

  digitalWrite(COB_OUTPUT_PIN, outputLevel);

  if (!hasLoggedCobLevel || lastCobOn != cobOn)
  {
    Serial.print("HardwareOutputs COB GPIO");
    Serial.print(COB_OUTPUT_PIN);
    Serial.print(" -> ");
    Serial.print(outputLevel == HIGH ? "HIGH" : "LOW");
    Serial.print(" cob=");
    Serial.println(cobOn ? "ON" : "OFF");

    hasLoggedCobLevel = true;
    lastCobOn = cobOn;
  }
}
