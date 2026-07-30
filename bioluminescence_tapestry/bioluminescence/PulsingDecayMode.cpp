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
  _triggeredPin = 0;
  _I0_actual    = 0.0f;

  Serial.println("[PulsingDecay] Entered. Touch to trigger breathing glow.");
  Serial.print("  tau="); Serial.print(tau);
  Serial.print("ms  a="); Serial.print(a);
  Serial.print("  omega="); Serial.println(omega);
}

void PulsingDecayMode::onTouch(Adafruit_NeoPixel& strip,
                               const TouchEvent& event) {
  uint8_t pin = event.globalPin;
  bool isTouched = event.isTouched;
  int16_t pressure = event.pressure;

  if (pin >= NUM_PINS) return;
  if (!isTouched) return;

  _I0_actual    = constrain(map(pressure, 0, 150, 50, 255), 50, 255);
  _triggeredPin = pin;
  _startTime    = millis();
  _active       = true;

  Serial.print("[PulsingDecay] Touch pin="); Serial.print(pin);
  Serial.print("  I0="); Serial.println(_I0_actual);
}

void PulsingDecayMode::update(Adafruit_NeoPixel& strip) {
  if (!_active) return;

  float t_ms  = (float)(millis() - _startTime);
  float I     = computeIntensity(t_ms);

  // Clamp to [0, 255] – sine can temporarily push below zero
  uint8_t bri = intensityToBrightness(constrain(I, 0.0f, 255.0f));

  strip.fill(bioColor(strip, bri));
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
