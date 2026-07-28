#ifndef LIGHTING_MODE_H
#define LIGHTING_MODE_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Abstract base class representing a single lighting mode
class LightingMode {
public:
  virtual ~LightingMode() {}
  
  // Called when switching to this mode
  virtual void enter(Adafruit_NeoPixel& strip) = 0;
  
  // Called when a touch or release event occurs
  virtual void onTouch(Adafruit_NeoPixel& strip, uint8_t ch, uint8_t pin, bool isTouched) = 0;
  
  // Called continuously in loop() to update animation frame updates (e.g. flashing, breathing)
  virtual void update(Adafruit_NeoPixel& strip) = 0;
};

#endif
