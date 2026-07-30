// ============================================================
// SolidColorMode.h
// Diagnostic mode: each pin controls a fixed LED section.
// ============================================================
#ifndef SOLID_COLOR_MODE_H
#define SOLID_COLOR_MODE_H

#include "DecayMode.h"

class SolidColorMode : public DecayMode {
public:
  void enter(Adafruit_NeoPixel& strip) override;
  void onTouch(Adafruit_NeoPixel& strip,
               const TouchEvent& event) override;
  void update(Adafruit_NeoPixel& strip) override {}
  const char* getName() override { return "Solid Color (pin section test)"; }
};

#endif // SOLID_COLOR_MODE_H
