#include "LocalControls.h"

#if __has_include("HardwarePins.h")
#include "HardwarePins.h"
#else
#include "HardwarePins.example.h"
#endif

#ifndef LOCAL_PUMP_BUTTON_PIN
#define LOCAL_PUMP_BUTTON_PIN -1
#endif

#ifndef LOCAL_COB_BUTTON_PIN
#define LOCAL_COB_BUTTON_PIN -1
#endif

#ifndef LOCAL_BUTTONS_ACTIVE_LOW
#define LOCAL_BUTTONS_ACTIVE_LOW 1
#endif

#ifndef LOCAL_BUTTON_DEBOUNCE_MS
#define LOCAL_BUTTON_DEBOUNCE_MS 50
#endif

void LocalControls::begin()
{
  initializeButton(pumpButton, LOCAL_PUMP_BUTTON_PIN, "Pump");
  initializeButton(cobButton, LOCAL_COB_BUTTON_PIN, "COB");
}

void LocalControls::update()
{
  updateButton(pumpButton, "Pump");
  updateButton(cobButton, "COB");
}

bool LocalControls::consumePumpToggleRequest()
{
  bool requested = pumpButton.toggleRequested;
  pumpButton.toggleRequested = false;
  return requested;
}

bool LocalControls::consumeCobToggleRequest()
{
  bool requested = cobButton.toggleRequested;
  cobButton.toggleRequested = false;
  return requested;
}

void LocalControls::initializeButton(ButtonState &button, int pin, const char *label)
{
  button.pin = pin;
  button.lastRawPressed = false;
  button.debouncedPressed = false;
  button.lastRawChangedAt = millis();
  button.toggleRequested = false;

  if (pin < 0)
  {
    Serial.print(label);
    Serial.println(" local button not configured.");
    return;
  }

  pinMode(pin, LOCAL_BUTTONS_ACTIVE_LOW == 1 ? INPUT_PULLUP : INPUT);

  button.lastRawPressed = readPressed(pin);
  button.debouncedPressed = button.lastRawPressed;

  Serial.print(label);
  Serial.print(" local button initialized on GPIO");
  Serial.print(pin);
  Serial.print(" active_");
  Serial.print(LOCAL_BUTTONS_ACTIVE_LOW == 1 ? "LOW" : "HIGH");
  Serial.print(" debounce_ms=");
  Serial.println(LOCAL_BUTTON_DEBOUNCE_MS);
}

void LocalControls::updateButton(ButtonState &button, const char *label)
{
  if (button.pin < 0)
  {
    return;
  }

  bool rawPressed = readPressed(button.pin);
  unsigned long now = millis();

  if (rawPressed != button.lastRawPressed)
  {
    button.lastRawPressed = rawPressed;
    button.lastRawChangedAt = now;
  }

  if (button.debouncedPressed != rawPressed &&
      now - button.lastRawChangedAt >= LOCAL_BUTTON_DEBOUNCE_MS)
  {
    button.debouncedPressed = rawPressed;

    if (button.debouncedPressed)
    {
      button.toggleRequested = true;
      Serial.print(label);
      Serial.println(" local button pressed.");
    }
  }
}

bool LocalControls::readPressed(int pin) const
{
  int state = digitalRead(pin);
  return LOCAL_BUTTONS_ACTIVE_LOW == 1 ? state == LOW : state == HIGH;
}
