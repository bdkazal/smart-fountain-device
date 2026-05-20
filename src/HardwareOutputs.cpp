#include "HardwareOutputs.h"

#if __has_include("HardwarePins.h")
#include "HardwarePins.h"
#else
#include "HardwarePins.example.h"
#endif

#ifndef PUMP_OUTPUT_ACTIVE_HIGH
#define PUMP_OUTPUT_ACTIVE_HIGH 1
#endif

#ifndef PUMP_PWM_ENABLED
#define PUMP_PWM_ENABLED 0
#endif

#ifndef PUMP_PWM_CHANNEL
#define PUMP_PWM_CHANNEL 1
#endif

#ifndef PUMP_PWM_FREQUENCY
#define PUMP_PWM_FREQUENCY 20000
#endif

#ifndef PUMP_PWM_RESOLUTION_BITS
#define PUMP_PWM_RESOLUTION_BITS 8
#endif

#ifndef PUMP_PWM_MIN_DUTY_PERCENT
#define PUMP_PWM_MIN_DUTY_PERCENT 10
#endif

#ifndef PUMP_PWM_MAX_DUTY_PERCENT
#define PUMP_PWM_MAX_DUTY_PERCENT 100
#endif

#ifndef PUMP_STARTUP_BOOST_DUTY_PERCENT
#define PUMP_STARTUP_BOOST_DUTY_PERCENT 100
#endif

#ifndef PUMP_STARTUP_BOOST_MS
#define PUMP_STARTUP_BOOST_MS 700
#endif

void HardwareOutputs::begin()
{
  hardwareEnabled = SMART_FOUNTAIN_HARDWARE_ENABLED == 1;

  if (!hardwareEnabled)
  {
    Serial.println("HardwareOutputs disabled: running software-only output state.");
    return;
  }

  if (PUMP_OUTPUT_PIN >= 0)
  {
    if (PUMP_PWM_ENABLED == 1)
    {
      ledcSetup(PUMP_PWM_CHANNEL, PUMP_PWM_FREQUENCY, PUMP_PWM_RESOLUTION_BITS);
      ledcAttachPin(PUMP_OUTPUT_PIN, PUMP_PWM_CHANNEL);
      ledcWrite(PUMP_PWM_CHANNEL, PUMP_OUTPUT_ACTIVE_HIGH == 1 ? 0 : ((1 << PUMP_PWM_RESOLUTION_BITS) - 1));

      Serial.print("HardwareOutputs pump PWM ready: GPIO ");
      Serial.print(PUMP_OUTPUT_PIN);
      Serial.print(" channel=");
      Serial.print(PUMP_PWM_CHANNEL);
      Serial.print(" freq=");
      Serial.print(PUMP_PWM_FREQUENCY);
      Serial.print("Hz active_");
      Serial.print(PUMP_OUTPUT_ACTIVE_HIGH == 1 ? "HIGH" : "LOW");
      Serial.print(" min=");
      Serial.print(PUMP_PWM_MIN_DUTY_PERCENT);
      Serial.print(" max=");
      Serial.print(PUMP_PWM_MAX_DUTY_PERCENT);
      Serial.print(" boost_ms=");
      Serial.println(PUMP_STARTUP_BOOST_MS);
    }
    else
    {
      pinMode(PUMP_OUTPUT_PIN, OUTPUT);
      digitalWrite(PUMP_OUTPUT_PIN, PUMP_OUTPUT_ACTIVE_HIGH == 1 ? LOW : HIGH);
      Serial.print("HardwareOutputs pump pin ready: GPIO ");
      Serial.print(PUMP_OUTPUT_PIN);
      Serial.print(" active_");
      Serial.println(PUMP_OUTPUT_ACTIVE_HIGH == 1 ? "HIGH" : "LOW");
    }
  }
  else
  {
    Serial.println("HardwareOutputs pump pin not configured.");
  }

  if (COB_PWM_PIN >= 0)
  {
    ledcSetup(COB_PWM_CHANNEL, COB_PWM_FREQUENCY, COB_PWM_RESOLUTION_BITS);
    ledcAttachPin(COB_PWM_PIN, COB_PWM_CHANNEL);
    ledcWrite(COB_PWM_CHANNEL, 0);
    Serial.print("HardwareOutputs COB PWM ready: GPIO ");
    Serial.println(COB_PWM_PIN);
  }
  else
  {
    Serial.println("HardwareOutputs COB PWM pin not configured.");
  }

  Serial.println("HardwareOutputs RGB driver is placeholder/TBD.");
}

void HardwareOutputs::apply(const FountainOutputState &outputs)
{
  if (!hardwareEnabled)
  {
    return;
  }

  applyPump(outputs);
  applyCob(outputs);
  applyRgb(outputs);
}

bool HardwareOutputs::isEnabled() const
{
  return hardwareEnabled;
}

void HardwareOutputs::applyPump(const FountainOutputState &outputs)
{
  if (PUMP_OUTPUT_PIN < 0)
  {
    return;
  }

  bool pumpOn = outputs.pumpEnabled && outputs.pumpSpeedPercent > 0;
  int requestedSpeedPercent = pumpOn ? constrain(outputs.pumpSpeedPercent, 1, 100) : 0;

  if (PUMP_PWM_ENABLED == 1)
  {
    int dutyMax = (1 << PUMP_PWM_RESOLUTION_BITS) - 1;
    int minDutyPercent = constrain(PUMP_PWM_MIN_DUTY_PERCENT, 0, 100);
    int maxDutyPercent = constrain(PUMP_PWM_MAX_DUTY_PERCENT, minDutyPercent, 100);
    int mappedDutyPercent = 0;

    if (pumpOn)
    {
      // Direct pump calibration: dashboard speed is the actual PWM duty percent.
      // Only 1-9 are clamped up to the minimum so the pump is not asked to run
      // below its observed usable range.
      mappedDutyPercent = constrain(requestedSpeedPercent, minDutyPercent, maxDutyPercent);
    }

    if (pumpOn && !lastPumpOn)
    {
      pumpStartupBoostActive = PUMP_STARTUP_BOOST_MS > 0;
      pumpStartupBoostUntil = millis() + PUMP_STARTUP_BOOST_MS;
    }

    if (!pumpOn)
    {
      pumpStartupBoostActive = false;
      pumpStartupBoostUntil = 0;
    }

    bool boostActiveNow = pumpStartupBoostActive && millis() < pumpStartupBoostUntil;

    if (pumpStartupBoostActive && !boostActiveNow)
    {
      pumpStartupBoostActive = false;
    }

    int appliedDutyPercent = boostActiveNow
                                 ? constrain(PUMP_STARTUP_BOOST_DUTY_PERCENT, 0, 100)
                                 : mappedDutyPercent;
    int duty = map(appliedDutyPercent, 0, 100, 0, dutyMax);

    if (PUMP_OUTPUT_ACTIVE_HIGH != 1)
    {
      duty = dutyMax - duty;
    }

    duty = constrain(duty, 0, dutyMax);
    ledcWrite(PUMP_PWM_CHANNEL, duty);

    if (!hasLoggedPumpLevel ||
        lastPumpOn != pumpOn ||
        lastPumpSpeedPercent != requestedSpeedPercent ||
        lastPumpDuty != duty)
    {
      Serial.print("HardwareOutputs pump GPIO");
      Serial.print(PUMP_OUTPUT_PIN);
      Serial.print(" PWM duty=");
      Serial.print(duty);
      Serial.print("/");
      Serial.print(dutyMax);
      Serial.print(" pump=");
      Serial.print(pumpOn ? "ON" : "OFF");
      Serial.print(" requested_speed=");
      Serial.print(requestedSpeedPercent);
      Serial.print(" mapped_duty_percent=");
      Serial.print(mappedDutyPercent);
      Serial.print(" boost=");
      Serial.println(boostActiveNow ? "yes" : "no");

      hasLoggedPumpLevel = true;
      lastPumpOn = pumpOn;
      lastPumpSpeedPercent = requestedSpeedPercent;
      lastPumpDuty = duty;
    }

    return;
  }

  bool gpioOn = PUMP_OUTPUT_ACTIVE_HIGH == 1;
  int onLevel = gpioOn ? HIGH : LOW;
  int offLevel = gpioOn ? LOW : HIGH;
  int outputLevel = pumpOn ? onLevel : offLevel;

  digitalWrite(PUMP_OUTPUT_PIN, outputLevel);

  if (!hasLoggedPumpLevel || lastPumpOn != pumpOn)
  {
    Serial.print("HardwareOutputs pump GPIO");
    Serial.print(PUMP_OUTPUT_PIN);
    Serial.print(" -> ");
    Serial.print(outputLevel == HIGH ? "HIGH" : "LOW");
    Serial.print(" pump=");
    Serial.print(pumpOn ? "ON" : "OFF");
    Serial.print(" speed=");
    Serial.println(requestedSpeedPercent);

    hasLoggedPumpLevel = true;
    lastPumpOn = pumpOn;
  }
}

void HardwareOutputs::applyCob(const FountainOutputState &outputs)
{
  if (COB_PWM_PIN < 0)
  {
    return;
  }

  int dutyMax = (1 << COB_PWM_RESOLUTION_BITS) - 1;
  int duty = outputs.cobEnabled ? map(outputs.cobBrightnessPercent, 0, 100, 0, dutyMax) : 0;
  ledcWrite(COB_PWM_CHANNEL, constrain(duty, 0, dutyMax));
}

void HardwareOutputs::applyRgb(const FountainOutputState &outputs)
{
  // RGB hardware is intentionally not guessed yet. We need to know whether the
  // final module is addressable RGB, three-channel PWM RGB, or another driver.
  (void)outputs;
}
