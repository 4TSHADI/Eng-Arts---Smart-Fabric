// ============================================================
// SolidColorMode.cpp
// ============================================================
#include "SolidColorMode.h"

void SolidColorMode::enter(Adafruit_NeoPixel& strip) {
  strip.setBrightness(60);
  strip.clear();
  strip.show();
}

void SolidColorMode::onTouch(Adafruit_NeoPixel& strip,
                              uint8_t pin, bool isTouched, int16_t pressure) {
  if (pin >= NUM_PINS) return;

  PinSection s = getPinSection(pin);
  uint32_t color = isTouched ? strip.Color(0, 255, 0) : strip.Color(0, 0, 0);

  for (uint8_t y = s.yStart; y <= s.yEnd; y++) {
    for (uint8_t x = s.xStart; x <= s.xEnd; x++) {
      uint16_t idx = XY(x, y);
      if (idx < NUM_LEDS) strip.setPixelColor(idx, color);
    }
  }
  strip.show();
}
