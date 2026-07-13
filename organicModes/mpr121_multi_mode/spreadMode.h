#ifndef SPREAD_MODE_H
#define SPREAD_MODE_H

#include "LightingMode.h"
#include "ProjectConfig.h"

#define LEDnum 256
// SpreadMode lights up the corresponding region on touch with Green
class SpreadMode : public LightingMode {
public:
  void enter(Adafruit_NeoPixel& strip) override;
  void onTouch(Adafruit_NeoPixel& strip, uint8_t pin, bool isTouched, int16_t pressure) override;
  void update(Adafruit_NeoPixel& strip) override;
  const char* getName() override { return "Spread Mode"; }
};

#endif
