#ifndef BIOLUMINESCENT_MODE_H
#define BIOLUMINESCENT_MODE_H

#include "LightingMode.h"

// BioluminescentMode lights up the corresponding region on touch with Cyan
class BioluminescentMode : public LightingMode {
public:
  void enter(Adafruit_NeoPixel& strip) override;
  void onTouch(Adafruit_NeoPixel& strip, uint8_t pin, bool isTouched, int16_t pressure) override;
  void update(Adafruit_NeoPixel& strip) override;
  const char* getName() override { return "Bioluminescent Mode"; }
};

#endif
