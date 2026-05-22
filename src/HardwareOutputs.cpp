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

#ifndef PUMP_LOW_ASSIST_ENABLED
#define PUMP_LOW_ASSIST_ENABLED 0
#endif

#ifndef PUMP_LOW_ASSIST_MAX_SPEED_PERCENT
#define PUMP_LOW_ASSIST_MAX_SPEED_PERCENT 10
#endif

#ifndef PUMP_LOW_ASSIST_DUTY_PERCENT
#define PUMP_LOW_ASSIST_DUTY_PERCENT 100
#endif

#ifndef PUMP_LOW_ASSIST_KICK_MS
#define PUMP_LOW_ASSIST_KICK_MS 80
#endif

#ifndef PUMP_LOW_ASSIST_INTERVAL_MS
#define PUMP_LOW_ASSIST_INTERVAL_MS 2000
#endif

#ifndef COB_OUTPUT_PIN
#define COB_OUTPUT_PIN -1
#endif

#ifndef COB_OUTPUT_ACTIVE_HIGH
#define COB_OUTPUT_ACTIVE_HIGH 1
#endif

#ifndef RGB_HARDWARE_TYPE_NONE
#define RGB_HARDWARE_TYPE_NONE 0
#endif

#ifndef RGB_HARDWARE_TYPE_ANALOG_PWM
#define RGB_HARDWARE_TYPE_ANALOG_PWM 1
#endif

#ifndef RGB_HARDWARE_TYPE
#define RGB_HARDWARE_TYPE RGB_HARDWARE_TYPE_NONE
#endif

#ifndef RGB_RED_PIN
#define RGB_RED_PIN -1
#endif

#ifndef RGB_GREEN_PIN
#define RGB_GREEN_PIN -1
#endif

#ifndef RGB_BLUE_PIN
#define RGB_BLUE_PIN -1
#endif

#ifndef RGB_RED_CHANNEL
#define RGB_RED_CHANNEL 2
#endif

#ifndef RGB_GREEN_CHANNEL
#define RGB_GREEN_CHANNEL 3
#endif

#ifndef RGB_BLUE_CHANNEL
#define RGB_BLUE_CHANNEL 4
#endif

#ifndef RGB_PWM_FREQUENCY
#define RGB_PWM_FREQUENCY 1000
#endif

#ifndef RGB_PWM_RESOLUTION_BITS
#define RGB_PWM_RESOLUTION_BITS 8
#endif

#ifndef RGB_OUTPUT_ACTIVE_HIGH
#define RGB_OUTPUT_ACTIVE_HIGH 1
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
      Serial.print(PUMP_STARTUP_BOOST_MS);
      Serial.print(" low_assist=");
      Serial.println(PUMP_LOW_ASSIST_ENABLED == 1 ? "on" : "off");
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

  if (COB_OUTPUT_PIN >= 0)
  {
    pinMode(COB_OUTPUT_PIN, OUTPUT);
    digitalWrite(COB_OUTPUT_PIN, COB_OUTPUT_ACTIVE_HIGH == 1 ? LOW : HIGH);
    Serial.print("HardwareOutputs COB pin ready: GPIO ");
    Serial.print(COB_OUTPUT_PIN);
    Serial.print(" active_");
    Serial.println(COB_OUTPUT_ACTIVE_HIGH == 1 ? "HIGH" : "LOW");
  }
  else
  {
    Serial.println("HardwareOutputs COB pin not configured.");
  }

  if (RGB_HARDWARE_TYPE == RGB_HARDWARE_TYPE_ANALOG_PWM && RGB_RED_PIN >= 0 && RGB_GREEN_PIN >= 0 && RGB_BLUE_PIN >= 0)
  {
    int offDuty = RGB_OUTPUT_ACTIVE_HIGH == 1 ? 0 : ((1 << RGB_PWM_RESOLUTION_BITS) - 1);

    ledcSetup(RGB_RED_CHANNEL, RGB_PWM_FREQUENCY, RGB_PWM_RESOLUTION_BITS);
    ledcSetup(RGB_GREEN_CHANNEL, RGB_PWM_FREQUENCY, RGB_PWM_RESOLUTION_BITS);
    ledcSetup(RGB_BLUE_CHANNEL, RGB_PWM_FREQUENCY, RGB_PWM_RESOLUTION_BITS);

    ledcAttachPin(RGB_RED_PIN, RGB_RED_CHANNEL);
    ledcAttachPin(RGB_GREEN_PIN, RGB_GREEN_CHANNEL);
    ledcAttachPin(RGB_BLUE_PIN, RGB_BLUE_CHANNEL);

    ledcWrite(RGB_RED_CHANNEL, offDuty);
    ledcWrite(RGB_GREEN_CHANNEL, offDuty);
    ledcWrite(RGB_BLUE_CHANNEL, offDuty);

    Serial.print("HardwareOutputs analog RGB PWM ready: R=GPIO");
    Serial.print(RGB_RED_PIN);
    Serial.print(" G=GPIO");
    Serial.print(RGB_GREEN_PIN);
    Serial.print(" B=GPIO");
    Serial.print(RGB_BLUE_PIN);
    Serial.print(" freq=");
    Serial.print(RGB_PWM_FREQUENCY);
    Serial.print("Hz active_");
    Serial.println(RGB_OUTPUT_ACTIVE_HIGH == 1 ? "HIGH" : "LOW");
  }
  else
  {
    Serial.println("HardwareOutputs RGB driver is placeholder/TBD.");
  }
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
      mappedDutyPercent = constrain(requestedSpeedPercent, minDutyPercent, maxDutyPercent);
    }

    if (pumpOn && !lastPumpOn)
    {
      pumpStartupBoostActive = PUMP_STARTUP_BOOST_MS > 0;
      pumpStartupBoostUntil = millis() + PUMP_STARTUP_BOOST_MS;
      lastPumpAssistKickAt = millis();
    }

    if (!pumpOn)
    {
      pumpStartupBoostActive = false;
      pumpStartupBoostUntil = 0;
      lastPumpAssistKickAt = 0;
    }

    unsigned long now = millis();
    bool boostActiveNow = pumpStartupBoostActive && now < pumpStartupBoostUntil;

    if (pumpStartupBoostActive && !boostActiveNow)
    {
      pumpStartupBoostActive = false;
    }

    bool lowAssistActiveNow = false;

    if (PUMP_LOW_ASSIST_ENABLED == 1 &&
        pumpOn &&
        !boostActiveNow &&
        requestedSpeedPercent <= PUMP_LOW_ASSIST_MAX_SPEED_PERCENT)
    {
      unsigned long elapsedSinceKick = now - lastPumpAssistKickAt;

      if (elapsedSinceKick >= PUMP_LOW_ASSIST_INTERVAL_MS)
      {
        lastPumpAssistKickAt = now;
        elapsedSinceKick = 0;
      }

      lowAssistActiveNow = elapsedSinceKick < PUMP_LOW_ASSIST_KICK_MS;
    }

    int appliedDutyPercent = mappedDutyPercent;

    if (boostActiveNow)
    {
      appliedDutyPercent = constrain(PUMP_STARTUP_BOOST_DUTY_PERCENT, 0, 100);
    }
    else if (lowAssistActiveNow)
    {
      appliedDutyPercent = constrain(PUMP_LOW_ASSIST_DUTY_PERCENT, 0, 100);
    }

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
        lastPumpDuty != duty ||
        lastPumpAssistActive != lowAssistActiveNow)
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
      Serial.print(boostActiveNow ? "yes" : "no");
      Serial.print(" assist=");
      Serial.println(lowAssistActiveNow ? "yes" : "no");

      hasLoggedPumpLevel = true;
      lastPumpOn = pumpOn;
      lastPumpSpeedPercent = requestedSpeedPercent;
      lastPumpDuty = duty;
      lastPumpAssistActive = lowAssistActiveNow;
    }

    return;
  }

  bool activeHigh = PUMP_OUTPUT_ACTIVE_HIGH == 1;
  int onLevel = activeHigh ? HIGH : LOW;
  int offLevel = activeHigh ? LOW : HIGH;
  int outputLevel = pumpOn ? onLevel : offLevel;

  digitalWrite(PUMP_OUTPUT_PIN, outputLevel);

  if (!hasLoggedPumpLevel || lastPumpOn != pumpOn)
  {
    Serial.print("HardwareOutputs pump GPIO");
    Serial.print(PUMP_OUTPUT_PIN);
    Serial.print(" -> ");
    Serial.print(outputLevel == HIGH ? "HIGH" : "LOW");
    Serial.print(" pump=");
    Serial.println(pumpOn ? "ON" : "OFF");

    hasLoggedPumpLevel = true;
    lastPumpOn = pumpOn;
  }
}

void HardwareOutputs::applyCob(const FountainOutputState &outputs)
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

void HardwareOutputs::applyRgb(const FountainOutputState &outputs)
{
  if (RGB_HARDWARE_TYPE != RGB_HARDWARE_TYPE_ANALOG_PWM || RGB_RED_PIN < 0 || RGB_GREEN_PIN < 0 || RGB_BLUE_PIN < 0)
  {
    return;
  }

  int red = 0;
  int green = 0;
  int blue = 0;
  int brightness = outputs.rgbEnabled ? constrain(outputs.rgbBrightnessPercent, 0, 100) : 0;
  String effect = outputs.rgbEffect;

  parseRgbColor(outputs.rgbColor, red, green, blue);

  if (outputs.rgbEnabled && effect == "breathing")
  {
    float phase = (millis() % 3000) / 3000.0f;
    float wave = (sin(phase * 2.0f * PI) + 1.0f) / 2.0f;
    brightness = (int)(brightness * (0.20f + (0.80f * wave)));
  }

  int redDuty = rgbDutyFromChannel(red, brightness);
  int greenDuty = rgbDutyFromChannel(green, brightness);
  int blueDuty = rgbDutyFromChannel(blue, brightness);

  ledcWrite(RGB_RED_CHANNEL, redDuty);
  ledcWrite(RGB_GREEN_CHANNEL, greenDuty);
  ledcWrite(RGB_BLUE_CHANNEL, blueDuty);

  if (!hasLoggedRgbLevel ||
      lastRgbOn != outputs.rgbEnabled ||
      lastRgbBrightnessPercent != outputs.rgbBrightnessPercent ||
      lastRgbColor != outputs.rgbColor ||
      lastRgbEffect != outputs.rgbEffect ||
      lastRgbRedDuty != redDuty ||
      lastRgbGreenDuty != greenDuty ||
      lastRgbBlueDuty != blueDuty)
  {
    Serial.print("HardwareOutputs RGB analog PWM R=GPIO");
    Serial.print(RGB_RED_PIN);
    Serial.print(" duty=");
    Serial.print(redDuty);
    Serial.print(" G=GPIO");
    Serial.print(RGB_GREEN_PIN);
    Serial.print(" duty=");
    Serial.print(greenDuty);
    Serial.print(" B=GPIO");
    Serial.print(RGB_BLUE_PIN);
    Serial.print(" duty=");
    Serial.print(blueDuty);
    Serial.print(" enabled=");
    Serial.print(outputs.rgbEnabled ? "true" : "false");
    Serial.print(" brightness=");
    Serial.print(outputs.rgbBrightnessPercent);
    Serial.print(" color=");
    Serial.print(outputs.rgbColor);
    Serial.print(" effect=");
    Serial.println(outputs.rgbEffect);

    hasLoggedRgbLevel = true;
    lastRgbOn = outputs.rgbEnabled;
    lastRgbBrightnessPercent = outputs.rgbBrightnessPercent;
    lastRgbColor = outputs.rgbColor;
    lastRgbEffect = outputs.rgbEffect;
    lastRgbRedDuty = redDuty;
    lastRgbGreenDuty = greenDuty;
    lastRgbBlueDuty = blueDuty;
  }
}

int HardwareOutputs::rgbDutyFromChannel(int channelValue, int brightnessPercent) const
{
  int dutyMax = (1 << RGB_PWM_RESOLUTION_BITS) - 1;
  int duty = map(constrain(channelValue, 0, 255), 0, 255, 0, dutyMax);
  duty = (duty * constrain(brightnessPercent, 0, 100)) / 100;

  if (RGB_OUTPUT_ACTIVE_HIGH != 1)
  {
    duty = dutyMax - duty;
  }

  return constrain(duty, 0, dutyMax);
}

void HardwareOutputs::parseRgbColor(const String &hexColor, int &red, int &green, int &blue) const
{
  String color = hexColor;
  color.trim();

  if (color.length() != 7 || color.charAt(0) != '#')
  {
    red = 0;
    green = 0;
    blue = 0;
    return;
  }

  red = (int)strtol(color.substring(1, 3).c_str(), nullptr, 16);
  green = (int)strtol(color.substring(3, 5).c_str(), nullptr, 16);
  blue = (int)strtol(color.substring(5, 7).c_str(), nullptr, 16);
}
