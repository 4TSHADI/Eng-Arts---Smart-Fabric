// ============================================================
// PulsingDecayMode.cpp
// MODEL 2 – I(t) = I₀ · e^(−t/τ) · (1 + a·sin(ωt))
// ============================================================
#include "PulsingDecayMode.h"

// ── Lifecycle ─────────────────────────────────────────────────

void PulsingDecayMode::enter(Adafruit_NeoPixel& strip) {
  strip.clear();
  strip.show();

  _active       = false;
  _startTime    = 0;
  _triggeredPoint = 0;
  _I0_actual    = 0.0f;

  Serial.println("[PulsingDecay] Entered. Touch to trigger breathing glow.");
  Serial.print("  tau="); Serial.print(tau);
  Serial.print("ms  a="); Serial.print(a);
  Serial.print("  omega="); Serial.println(omega);
}

void PulsingDecayMode::onTouch(Adafruit_NeoPixel& strip,
                                const TouchEvent& event) {
  if (!event.isTouched) return;
  if (event.xCell >= TOUCH_GRID_SIZE || event.yCell >= TOUCH_GRID_SIZE) return;

  _I0_actual = constrain(map(event.pressure, 0, 150, 50, 255), 50, 255);
  _triggeredPoint = touchPointIndex(event.panelId, event.xCell, event.yCell);
  _startTime = millis();
  _active = true;

  Serial.print("[PulsingDecay] panel="); Serial.print(event.panelId);
  Serial.print(" x="); Serial.print(event.xCell);
  Serial.print(" y="); Serial.print(event.yCell);
  Serial.print("  I0="); Serial.println(_I0_actual);
}

void PulsingDecayMode::update(Adafruit_NeoPixel& strip) {
  if (!_active) return;

  float t_ms  = (float)(millis() - _startTime);
  float I     = computeIntensity(t_ms);

  uint8_t bri = intensityToBrightness(constrain(I, 0.0f, 255.0f));

  PinSection section = getTouchRegionBounds(touchPointPanel(_triggeredPoint), touchPointX(_triggeredPoint), touchPointY(_triggeredPoint));
  strip.clear();
  for (uint8_t y = section.yStart; y <= section.yEnd; y++) {
    for (uint8_t x = section.xStart; x <= section.xEnd; x++) {
      uint16_t idx = XY(x, y);
      if (idx < NUM_LEDS) strip.setPixelColor(idx, bioColor(strip, bri));
    }
  }
  strip.show();

  // Decay complete when the exponential envelope is negligible
  float envelope = _I0_actual * expf(-t_ms / tau);
  if (envelope < 1.0f) {
    _active = false;
    strip.clear();
    strip.show();
    Serial.println("[PulsingDecay] Decay complete.");
  }
}

// ── Math ──────────────────────────────────────────────────────

float PulsingDecayMode::computeIntensity(float t_ms) {
  //  I(t) = I₀ · e^(−t/τ) · (1 + a·sin(ωt))
  float envelope = _I0_actual * expf(-t_ms / tau);
  float modulator = 1.0f + a * sinf(omega * t_ms);
  return envelope * modulator;
}
