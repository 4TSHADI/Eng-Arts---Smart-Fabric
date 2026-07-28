#ifndef LOW_BRIGHTNESS_MODE_H
#define LOW_BRIGHTNESS_MODE_H

#include "LightingMode.h"
#include "ProjectConfig.h"

// LowBrightnessMode lights up regions with a dim visual response
class LowBrightnessMode : public LightingMode {
public:
  void enter(Adafruit_NeoPixel& strip) override;
  void onTouch(Adafruit_NeoPixel& strip, uint8_t ch, uint8_t pin, bool isTouched) override;
  void update(Adafruit_NeoPixel& strip) override;
};

#endif
