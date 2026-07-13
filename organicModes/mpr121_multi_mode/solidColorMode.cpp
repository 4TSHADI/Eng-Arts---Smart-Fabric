#include "solidColorMode.h"



void SolidColorMode::enter(Adafruit_NeoPixel& strip) {
  strip.setBrightness(50); // Set standard brightness
  strip.clear();
  strip.show();
}

void SolidColorMode::onTouch(Adafruit_NeoPixel& strip, uint8_t pin, bool isTouched, int16_t pressure) = 0;
 {
  if (pin >= 12) return;

  // For Solid Color Mode, set region to Red on touch, off on release
  uint32_t color = isTouched ? strip.Color(0, 255, 0) : strip.Color(0, 0, 0);
  Region r = regions[pin];

  for (uint8_t y = r.yStart; y <= r.yEnd; y++) {
    for (uint8_t x = r.xStart; x <= r.xEnd; x++) {
      uint16_t idx = XY(x, y);
      if (idx < NUM_LEDS) {
        strip.setPixelColor(idx, color);
      }
    }
  }
  // center
  Point p = centerCoordinates[pin];
  uint16_t idx = XY(p.x,p.y);

  strip.setPixelColor(idx, strip.Color(0, 0, 255));

  strip.show();
}

void SolidColorMode::update(Adafruit_NeoPixel& strip) {
  // Static color mode doesn't need temporal updates
}
