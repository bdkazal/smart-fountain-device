#include "RgbOutputDriver.h"

#include <FastLED.h>

#if __has_include("HardwarePins.h")
#include "HardwarePins.h"
#else
#include "HardwarePins.example.h"
#endif

#ifndef RGB_HARDWARE_TYPE_NEOPIXEL
#define RGB_HARDWARE_TYPE_NEOPIXEL 2
#endif

#ifndef RGB_HARDWARE_TYPE
#define RGB_HARDWARE_TYPE RGB_HARDWARE_TYPE_NEOPIXEL
#endif

#ifndef RGB_GREEN_CALIBRATION_PERCENT
#define RGB_GREEN_CALIBRATION_PERCENT 100
#endif

#ifndef NEOPIXEL_DATA_PIN
#define NEOPIXEL_DATA_PIN 27
#endif

#ifndef NEOPIXEL_COUNT
#define NEOPIXEL_COUNT 12
#endif

#ifndef NEOPIXEL_MAX_BRIGHTNESS
#define NEOPIXEL_MAX_BRIGHTNESS 160
#endif

#ifndef NEOPIXEL_COLOR_ORDER_GRB
#define NEOPIXEL_COLOR_ORDER_GRB 1
#endif

#ifndef NEOPIXEL_FRAME_INTERVAL_MS
#define NEOPIXEL_FRAME_INTERVAL_MS 50
#endif

#ifndef RGB_BREATHING_PERIOD_MS
#define RGB_BREATHING_PERIOD_MS 10000
#endif

#ifndef RGB_SLOW_RAINBOW_PERIOD_MS
#define RGB_SLOW_RAINBOW_PERIOD_MS 36000
#endif

#ifndef RGB_WARM_GLOW_PERIOD_MS
#define RGB_WARM_GLOW_PERIOD_MS 14000
#endif

#ifndef RGB_WATER_SHIMMER_PERIOD_MS
#define RGB_WATER_SHIMMER_PERIOD_MS 9000
#endif

namespace
{
CRGB pixels[NEOPIXEL_COUNT];
unsigned long lastFrameAt = 0;
bool frameRendered = false;
}

void RgbOutputDriver::begin()
{
  if (RGB_HARDWARE_TYPE != RGB_HARDWARE_TYPE_NEOPIXEL || NEOPIXEL_COUNT <= 0)
  {
    Serial.println("HardwareOutputs RGB driver is placeholder/TBD.");
    return;
  }

#if NEOPIXEL_COLOR_ORDER_GRB == 1
  FastLED.addLeds<WS2812B, NEOPIXEL_DATA_PIN, GRB>(pixels, NEOPIXEL_COUNT);
#else
  FastLED.addLeds<WS2812B, NEOPIXEL_DATA_PIN, RGB>(pixels, NEOPIXEL_COUNT);
#endif
  FastLED.setBrightness(constrain(NEOPIXEL_MAX_BRIGHTNESS, 0, 255));
  renderNeoPixels(0, 0, 0);

  Serial.print("HardwareOutputs NeoPixel ready: data=GPIO");
  Serial.print(NEOPIXEL_DATA_PIN);
  Serial.print(" count=");
  Serial.print(NEOPIXEL_COUNT);
  Serial.print(" max_brightness=");
  Serial.print(NEOPIXEL_MAX_BRIGHTNESS);
  Serial.print(" color_order=");
  Serial.println(NEOPIXEL_COLOR_ORDER_GRB == 1 ? "GRB" : "RGB");
  Serial.print("frame_interval_ms=");
  Serial.println(NEOPIXEL_FRAME_INTERVAL_MS);
  Serial.print("periods_ms breathing=");
  Serial.print(RGB_BREATHING_PERIOD_MS);
  Serial.print(" rainbow=");
  Serial.print(RGB_SLOW_RAINBOW_PERIOD_MS);
  Serial.print(" warm_glow=");
  Serial.print(RGB_WARM_GLOW_PERIOD_MS);
  Serial.print(" water_shimmer=");
  Serial.println(RGB_WATER_SHIMMER_PERIOD_MS);
  Serial.println("NeoPixel effects ready: solid, breathing, slow_rainbow, warm_glow, water_shimmer, night_mode.");
}

void RgbOutputDriver::apply(const FountainOutputState &outputs)
{
  if (RGB_HARDWARE_TYPE != RGB_HARDWARE_TYPE_NEOPIXEL || NEOPIXEL_COUNT <= 0)
  {
    return;
  }

  int red = 0;
  int green = 0;
  int blue = 0;
  int brightness = outputs.rgbEnabled ? constrain(outputs.rgbBrightnessPercent, 0, 100) : 0;
  String effect = outputs.rgbEffect;
  String hardwareMode = effect;

  parseRgbColor(outputs.rgbColor, red, green, blue);

  if (!outputs.rgbEnabled)
  {
    brightness = 0;
    hardwareMode = "off";
  }
  else if (effect == "breathing")
  {
    unsigned long period = safeEffectPeriod(RGB_BREATHING_PERIOD_MS, 10000);
    float phase = (millis() % period) / (float)period;
    float wave = (sin(phase * 2.0f * PI) + 1.0f) / 2.0f;
    brightness = (int)(brightness * (0.18f + (0.82f * wave)));
  }
  else if (effect == "slow_rainbow")
  {
    unsigned long period = safeEffectPeriod(RGB_SLOW_RAINBOW_PERIOD_MS, 36000);
    unsigned long segmentDuration = max(1UL, period / 6UL);
    unsigned long t = millis() % period;
    int segment = t / segmentDuration;
    int offset = map(t % segmentDuration, 0, segmentDuration - 1, 0, 255);

    switch (segment)
    {
    case 0: red = 255; green = offset; blue = 0; break;
    case 1: red = 255 - offset; green = 255; blue = 0; break;
    case 2: red = 0; green = 255; blue = offset; break;
    case 3: red = 0; green = 255 - offset; blue = 255; break;
    case 4: red = offset; green = 0; blue = 255; break;
    default: red = 255; green = 0; blue = 255 - offset; break;
    }
  }
  else if (effect == "warm_glow")
  {
    unsigned long period = safeEffectPeriod(RGB_WARM_GLOW_PERIOD_MS, 14000);
    float phase = (millis() % period) / (float)period;
    float wave = (sin(phase * 2.0f * PI) + 1.0f) / 2.0f;
    red = 255;
    green = 110 + (int)(50.0f * wave);
    blue = 18;
    brightness = (int)(brightness * (0.72f + (0.28f * wave)));
  }
  else if (effect == "water_shimmer")
  {
    unsigned long period = safeEffectPeriod(RGB_WATER_SHIMMER_PERIOD_MS, 9000);
    float phaseA = (millis() % period) / (float)period;
    float phaseB = ((millis() + (period / 3UL)) % period) / (float)period;
    float waveA = (sin(phaseA * 2.0f * PI) + 1.0f) / 2.0f;
    float waveB = (sin(phaseB * 2.0f * PI) + 1.0f) / 2.0f;
    red = 0;
    green = 95 + (int)(85.0f * waveA);
    blue = 170 + (int)(85.0f * waveB);
  }
  else if (effect == "night_mode")
  {
    red = 0;
    green = 8;
    blue = 80;
    brightness = min(brightness, 35);
  }
  else
  {
    hardwareMode = "solid";
  }

  int renderedRed = (constrain(red, 0, 255) * brightness) / 100;
  int renderedGreen = (constrain(green, 0, 255) * brightness) / 100;
  int renderedBlue = (constrain(blue, 0, 255) * brightness) / 100;
  bool animated = outputs.rgbEnabled && isAnimatedEffect(effect);
  bool logicalChanged = !hasLoggedRgbLevel || lastRgbOn != outputs.rgbEnabled || lastRgbBrightnessPercent != outputs.rgbBrightnessPercent || lastRgbColor != outputs.rgbColor || lastRgbEffect != outputs.rgbEffect;
  bool renderedChanged = !frameRendered || lastRgbRedDuty != renderedRed || lastRgbGreenDuty != renderedGreen || lastRgbBlueDuty != renderedBlue;

  if (!logicalChanged && !renderedChanged)
  {
    return;
  }

  if (animated && !logicalChanged && (millis() - lastFrameAt) < NEOPIXEL_FRAME_INTERVAL_MS)
  {
    return;
  }

  renderNeoPixels(renderedRed, renderedGreen, renderedBlue);

  if (logicalChanged)
  {
    Serial.print("HardwareOutputs NeoPixel rendered rgb=");
    Serial.print(renderedRed);
    Serial.print(",");
    Serial.print(renderedGreen);
    Serial.print(",");
    Serial.print(renderedBlue);
    Serial.print(" source_rgb=");
    Serial.print(red);
    Serial.print(",");
    Serial.print(green);
    Serial.print(",");
    Serial.print(blue);
    Serial.print(" mode=");
    Serial.print(hardwareMode);
    Serial.print(" data=GPIO");
    Serial.print(NEOPIXEL_DATA_PIN);
    Serial.print(" count=");
    Serial.print(NEOPIXEL_COUNT);
    Serial.print(" enabled=");
    Serial.print(outputs.rgbEnabled ? "true" : "false");
    Serial.print(" brightness=");
    Serial.print(outputs.rgbBrightnessPercent);
    Serial.print(" color=");
    Serial.print(outputs.rgbColor);
    Serial.print(" effect=");
    Serial.println(outputs.rgbEffect);
  }

  hasLoggedRgbLevel = true;
  lastRgbOn = outputs.rgbEnabled;
  lastRgbBrightnessPercent = outputs.rgbBrightnessPercent;
  lastRgbColor = outputs.rgbColor;
  lastRgbEffect = outputs.rgbEffect;
  lastRgbRedDuty = renderedRed;
  lastRgbGreenDuty = renderedGreen;
  lastRgbBlueDuty = renderedBlue;
}

bool RgbOutputDriver::isAnimatedEffect(const String &effect) const
{
  return effect == "breathing" || effect == "slow_rainbow" || effect == "warm_glow" || effect == "water_shimmer" || effect == "night_mode";
}

unsigned long RgbOutputDriver::safeEffectPeriod(unsigned long configuredPeriod, unsigned long fallbackPeriod) const
{
  return configuredPeriod >= 1000 ? configuredPeriod : fallbackPeriod;
}

void RgbOutputDriver::renderNeoPixels(int red, int green, int blue)
{
  fill_solid(pixels, NEOPIXEL_COUNT, CRGB(red, green, blue));
  FastLED.show();
  lastFrameAt = millis();
  frameRendered = true;
}

int RgbOutputDriver::rgbDutyFromChannel(int channelValue, int brightnessPercent) const
{
  return 0;
}

void RgbOutputDriver::parseRgbColor(const String &hexColor, int &red, int &green, int &blue) const
{
  String color = hexColor;
  color.trim();

  if (color.length() != 7 || color.charAt(0) != '#')
  {
    Serial.print("Invalid RGB color received: ");
    Serial.println(hexColor);
    red = 0;
    green = 0;
    blue = 0;
    return;
  }

  red = (int)strtol(color.substring(1, 3).c_str(), nullptr, 16);
  green = (int)strtol(color.substring(3, 5).c_str(), nullptr, 16);
  blue = (int)strtol(color.substring(5, 7).c_str(), nullptr, 16);
  green = (green * constrain(RGB_GREEN_CALIBRATION_PERCENT, 0, 100)) / 100;
}
