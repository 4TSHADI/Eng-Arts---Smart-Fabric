// ============================================================
// SolidColorMode.h
// Diagnostic mode: fills each electrode's 3×3 section solid
// green on touch, black on release.  Use this to verify the
// section mapping before flashing other modes.
// ============================================================
#ifndef SOLID_COLOR_MODE_H
#define SOLID_COLOR_MODE_H

#include "LightingMode.h"

class SolidColorMode : public LightingMode {
public:
  void enter  (Adafruit_NeoPixel& strip) override;
  void onTouch(Adafruit_NeoPixel& strip,
               const TouchEvent& event) override;
  void update (Adafruit_NeoPixel& strip) override {}
  const char* getName() override { return "Solid Color (section test)"; }
};

#endif // SOLID_COLOR_MODE_H
