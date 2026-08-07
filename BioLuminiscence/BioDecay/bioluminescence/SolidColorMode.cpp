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
                              const TouchEvent& event) {
  if (event.xCell >= TOUCH_GRID_SIZE || event.yCell >= TOUCH_GRID_SIZE) return;

  PinSection s = getTouchRegionBounds(event.panelId, event.xCell, event.yCell);
  uint32_t color = strip.Color(0, 0, 0);
  if (event.isTouched) {
    float spring = massSpringResponse(120.0f, event.pressure);
    float gain = 1.0f + 0.55f * spring;
    uint8_t g = (uint8_t)constrain((80.0f + (event.pressure * 1.1f)) * gain, 0.0f, 255.0f);
    color = strip.Color(0, g, 0);
  }

  for (uint8_t y = s.yStart; y <= s.yEnd; y++) {
    for (uint8_t x = s.xStart; x <= s.xEnd; x++) {
      uint16_t idx = XY(x, y);
      if (idx < NUM_LEDS) strip.setPixelColor(idx, color);
    }
  }
  strip.show();
}
