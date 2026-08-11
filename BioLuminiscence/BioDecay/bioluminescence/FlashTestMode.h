// ============================================================
// FlashTestMode.h
// Full-strip blink test for quickly validating LED output.
// ============================================================
#ifndef FLASH_TEST_MODE_H
#define FLASH_TEST_MODE_H

#include "LightingMode.h"

class FlashTestMode : public LightingMode {
public:
  void enter(Adafruit_NeoPixel& strip) override;
  void onTouch(Adafruit_NeoPixel& strip,
               const TouchEvent& event) override;
  void update(Adafruit_NeoPixel& strip) override;
  const char* getName() override { return "Flash Test"; }

private:
  bool _isOn = false;
  unsigned long _lastToggleMs = 0;
  static const uint16_t FLASH_INTERVAL_MS = 200;
  static const uint8_t TEST_BRIGHTNESS = 80;
};

#endif // FLASH_TEST_MODE_H