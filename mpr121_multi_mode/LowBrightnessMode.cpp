#include "LowBrightnessMode.h"

void LowBrightnessMode::enter(Adafruit_NeoPixel& strip) {
  strip.setBrightness(5); // Set very low brightness (e.g. 5 out of 255)
  strip.clear();
  strip.show();
}

void LowBrightnessMode::onTouch(Adafruit_NeoPixel& strip, uint8_t ch, uint8_t pin, bool isTouched) {
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

void LowBrightnessMode::update(Adafruit_NeoPixel& strip) {
  // Static low brightness mode, no updates needed
}
