// ============================================================
// FlashTestMode.cpp
// ============================================================
#include "FlashTestMode.h"

void FlashTestMode::enter(Adafruit_NeoPixel& strip) {
  strip.setBrightness(TEST_BRIGHTNESS);
  _isOn = false;
  _lastToggleMs = millis();
  strip.clear();
  strip.show();
}

void FlashTestMode::onTouch(Adafruit_NeoPixel& strip,
                            const TouchEvent& event) {
  (void)event;

  // Optional quick pulse trigger: any touch forces one immediate ON frame.
  _isOn = true;
  for (uint16_t i = 0; i < NUM_LEDS; ++i) {
    strip.setPixelColor(i, strip.Color(255, 255, 255));
  }
  strip.show();
  _lastToggleMs = millis();
}

void FlashTestMode::update(Adafruit_NeoPixel& strip) {
  unsigned long now = millis();
  if (now - _lastToggleMs < FLASH_INTERVAL_MS) return;

  _lastToggleMs = now;
  _isOn = !_isOn;

  uint32_t color = _isOn ? strip.Color(255, 255, 255) : strip.Color(0, 0, 0);
  for (uint16_t i = 0; i < NUM_LEDS; ++i) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}