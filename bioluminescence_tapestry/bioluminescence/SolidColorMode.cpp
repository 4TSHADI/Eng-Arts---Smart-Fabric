// ============================================================
// SolidColorMode.cpp
// ============================================================
#include "SolidColorMode.h"

void SolidColorMode::enter(Adafruit_NeoPixel& strip) {
  strip.setBrightness(60);
  strip.clear();
  strip.show();

  Serial.println("[SolidColor] Entered. Touch pins to verify section centers.");
}

void SolidColorMode::onTouch(Adafruit_NeoPixel& strip,
                             const TouchEvent& event) {
  if (event.electrode >= ELECTRODES_PER_MPR) return;

  PinPointGroup group = getSolidColorGroup(event.electrode);
  uint32_t color = event.isTouched ? strip.Color(0, 255, 0) : strip.Color(0, 0, 0);

  for (uint8_t i = 0; i < 4; i++) {
    uint16_t idx = XY(group.points[i].x, group.points[i].y);
    if (idx < NUM_LEDS) strip.setPixelColor(idx, color);
  }

  strip.show();
}
