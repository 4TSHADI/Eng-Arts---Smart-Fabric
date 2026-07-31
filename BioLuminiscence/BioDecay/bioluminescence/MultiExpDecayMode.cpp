// ============================================================
// MultiExpDecayMode.cpp
// MODEL 3 – I(t) = A₁·e^(−t/τ₁) + A₂·e^(−t/τ₂)
// ============================================================
#include "MultiExpDecayMode.h"

// ── Lifecycle ─────────────────────────────────────────────────

void MultiExpDecayMode::enter(Adafruit_NeoPixel& strip) {
  strip.clear();
  strip.show();

  _active       = false;
  _startTime    = 0;
  _triggeredPoint = 0;
  _scale        = 1.0f;

  Serial.println("[MultiExpDecay] Entered. Dinoflagellate bi-exponential model.");
  Serial.print("  Fast: A1="); Serial.print(A1); Serial.print("  tau1="); Serial.print(tau1); Serial.println("ms");
  Serial.print("  Slow: A2="); Serial.print(A2); Serial.print("  tau2="); Serial.print(tau2); Serial.println("ms");
}

void MultiExpDecayMode::onTouch(Adafruit_NeoPixel& strip,
                                 const TouchEvent& event) {
  if (!event.isTouched) return;
  if (event.xCell >= TOUCH_GRID_SIZE || event.yCell >= TOUCH_GRID_SIZE) return;

  _scale = constrain(map(event.pressure, 0, 150, 50, 100), 50, 100) / 100.0f;
  _triggeredPoint = touchPointIndex(event.panelId, event.xCell, event.yCell);
  _startTime = millis();
  _active = true;

  Serial.print("[MultiExpDecay] panel="); Serial.print(event.panelId);
  Serial.print(" x="); Serial.print(event.xCell);
  Serial.print(" y="); Serial.print(event.yCell);
  Serial.print(" pressure="); Serial.print(event.pressure);
  Serial.print("  scale="); Serial.println(_scale);
}

void MultiExpDecayMode::update(Adafruit_NeoPixel& strip) {
  if (!_active) return;

  float t_ms  = (float)(millis() - _startTime);
  float I     = computeIntensity(t_ms);
  uint8_t bri = intensityToBrightness(I);

  PinSection section = getTouchRegionBounds(touchPointPanel(_triggeredPoint), touchPointX(_triggeredPoint), touchPointY(_triggeredPoint));
  strip.clear();
  for (uint8_t y = section.yStart; y <= section.yEnd; y++) {
    for (uint8_t x = section.xStart; x <= section.xEnd; x++) {
      uint16_t idx = XY(x, y);
      if (idx < NUM_LEDS) strip.setPixelColor(idx, bioColor(strip, bri));
    }
  }
  strip.show();

  // Decay is complete when the slow component's contribution is negligible
  float slow = _scale * A2 * expf(-t_ms / tau2);
  if (slow < 1.0f && bri == 0) {
    _active = false;
    strip.clear();
    strip.show();
    Serial.println("[MultiExpDecay] Both components decayed. Done.");
  }
}

// ── Math ──────────────────────────────────────────────────────

float MultiExpDecayMode::computeIntensity(float t_ms) {
  //  I(t) = A₁·e^(−t/τ₁) + A₂·e^(−t/τ₂)
  float fast = _scale * A1 * expf(-t_ms / tau1);
  float slow = _scale * A2 * expf(-t_ms / tau2);
  return fast + slow;
}
