// ============================================================
// PulsingDecayMode.cpp
// MODEL 2 – SimpleDecay + pulse modulation
// I(t) = [SimpleDecay envelope] · (1 + a·sin(ωt))
// ============================================================
#include "PulsingDecayMode.h"

// ── Lifecycle ─────────────────────────────────────────────────

void PulsingDecayMode::enter(Adafruit_NeoPixel& strip) {
  SimpleDecayMode::enter(strip);

  Serial.println("[PulsingDecay] Entered. Inherits SimpleDecay, adds breathing pulse.");
  Serial.print("  tau="); Serial.print(tau);
  Serial.print("ms  a="); Serial.print(a);
  Serial.print("  omega="); Serial.println(omega);
}

// ── Math ──────────────────────────────────────────────────────

float PulsingDecayMode::computeIntensity(const TouchDecayState& state, float t_ms) {
  float envelope = state.I0_actual * expf(-t_ms / tau);
  float modulator = 1.0f + a * sinf(omega * t_ms);
  float spring = massSpringResponse(t_ms, state.pressure);
  float springGain = 1.0f + 0.60f * spring;
  return envelope * modulator * springGain;
}
