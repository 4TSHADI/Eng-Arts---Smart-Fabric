#ifndef SOLID_COLOR_MODE_H
#define SOLID_COLOR_MODE_H

#include "LightingMode.h"
#include "ProjectConfig.h"

// SolidColorMode lights up the corresponding region on touch with Red
class SolidColorMode : public LightingMode {
public:
  void enter(Adafruit_NeoPixel& strip) override;
  void onTouch(Adafruit_NeoPixel& strip, uint8_t pin, bool isTouched, int16_t pressure) override;
  void update(Adafruit_NeoPixel& strip) override;
  const char* getName() override { return "Solid Red Mode"; }
};

#endif
