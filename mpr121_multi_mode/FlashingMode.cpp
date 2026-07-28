#include "FlashingMode.h"

void FlashingMode::enter(Adafruit_NeoPixel& strip) {
  strip.setBrightness(50);
  strip.clear();
  strip.show();
  
  for (uint8_t ch = 0; ch < NUM_MUX_CHANNELS; ch++) {
    touchedPins[ch] = 0;
  }
  lastFlashTime = 0;
  flashOn = false;
}

void FlashingMode::onTouch(Adafruit_NeoPixel& strip, uint8_t ch, uint8_t pin, bool isTouched) {
  if (ch >= NUM_MUX_CHANNELS || pin >= 12) return;

  if (isTouched) {
    touchedPins[ch] |= (1 << pin); // Set pin bit
  } else {
    touchedPins[ch] &= ~(1 << pin); // Clear pin bit
  }
  
  // Instantly update when touched/released
  redraw(strip);
}

void FlashingMode::update(Adafruit_NeoPixel& strip) {
  unsigned long currentMillis = millis();
  if (currentMillis - lastFlashTime >= flashInterval) {
    lastFlashTime = currentMillis;
    flashOn = !flashOn; // Toggle visibility
    redraw(strip);
  }
}

void FlashingMode::redraw(Adafruit_NeoPixel& strip) {
  strip.clear();

  for (uint8_t ch = 0; ch < NUM_MUX_CHANNELS; ch++) {
    for (uint8_t pin = 0; pin < 12; pin++) {
      if (touchedPins[ch] & (1 << pin)) {
        // Pixel is touched: draw either color or black depending on flash state
        uint32_t color = flashOn ? getRegionColor(strip, ch) : 0;
        uint8_t offset1 = 2 * pin;
        uint8_t offset2 = 2 * pin + 1;

        uint8_t x1, y1, x2, y2;
        getRegionPixel(ch, offset1, x1, y1);
        getRegionPixel(ch, offset2, x2, y2);

        uint16_t idx1 = XY(x1, y1);
        uint16_t idx2 = XY(x2, y2);

        if (idx1 < NUM_LEDS) strip.setPixelColor(idx1, color);
        if (idx2 < NUM_LEDS) strip.setPixelColor(idx2, color);
      }
    }
  }
  strip.show();
}
