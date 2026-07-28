#include "SolidColorMode.h"

void SolidColorMode::enter(Adafruit_NeoPixel& strip) {
  strip.setBrightness(50); // Set standard brightness
  strip.clear();
  strip.show();
}

void SolidColorMode::onTouch(Adafruit_NeoPixel& strip, uint8_t ch, uint8_t pin, bool isTouched) {
  if (ch >= NUM_MUX_CHANNELS || pin >= 12) return;

  uint32_t color = isTouched ? getRegionColor(strip, ch) : 0;
  uint8_t offset1 = 2 * pin;
  uint8_t offset2 = 2 * pin + 1;

  uint8_t x1, y1, x2, y2;
  getRegionPixel(ch, offset1, x1, y1);
  getRegionPixel(ch, offset2, x2, y2);

  uint16_t idx1 = XY(x1, y1);
  uint16_t idx2 = XY(x2, y2);

  if (idx1 < NUM_LEDS) strip.setPixelColor(idx1, color);
  if (idx2 < NUM_LEDS) strip.setPixelColor(idx2, color);

  strip.show();
}

void SolidColorMode::update(Adafruit_NeoPixel& strip) {
  // Static color mode doesn't need temporal updates
}
