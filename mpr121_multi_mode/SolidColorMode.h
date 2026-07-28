#ifndef SOLID_COLOR_MODE_H
#define SOLID_COLOR_MODE_H

#include "LightingMode.h"
#include "ProjectConfig.h"

// SolidColorMode lights up the corresponding LEDs with the region's color
class SolidColorMode : public LightingMode {
public:
  void enter(Adafruit_NeoPixel& strip) override;
  void onTouch(Adafruit_NeoPixel& strip, uint8_t ch, uint8_t pin, bool isTouched) override;
  void update(Adafruit_NeoPixel& strip) override;
};

#endif
