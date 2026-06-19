#pragma once

#include <Arduino.h>

class LocalControls
{
public:
  void begin();
  void update();

  bool consumePumpToggleRequest();
  bool consumeCobToggleRequest();

private:
  struct ButtonState
  {
    int pin = -1;
    bool lastRawPressed = false;
    bool debouncedPressed = false;
    unsigned long lastRawChangedAt = 0;
    bool toggleRequested = false;
  };

  ButtonState pumpButton;
  ButtonState cobButton;

  void initializeButton(ButtonState &button, int pin, const char *label);
  void updateButton(ButtonState &button, const char *label);
  bool readPressed(int pin) const;
};
