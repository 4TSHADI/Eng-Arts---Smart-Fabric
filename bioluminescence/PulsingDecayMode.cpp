// ============================================================
// PulsingLightingMode.cpp
// MODEL 2 – SimpleLighting + pulse modulation
// I(t) = [SimpleLighting envelope] · (1 + a·sin(ωt))
// ============================================================
#include "PulsingLightingMode.h"

// ── Lifecycle ─────────────────────────────────────────────────

void PulsingLightingMode::enter(Adafruit_NeoPixel& strip) {
  SimpleLightingMode::enter(strip);

  if (MODE_EVENT_SERIAL_LOG) {
    Serial.println("[PulsingLighting] Entered. Inherits SimpleLighting, adds breathing pulse.");
    Serial.print("  tau="); Serial.print(tau);
    Serial.print("ms  a="); Serial.print(a);
    Serial.print("  omega="); Serial.println(omega);
  }
}

// ── Math ──────────────────────────────────────────────────────

float PulsingLightingMode::computeIntensity(const TouchLightingState& state, float t_ms) {
  float envelope = state.I0_actual * expf(-t_ms / tau);
  float modulator = 1.0f + a * sinf(omega * t_ms);
  float spring = massSpringResponse(t_ms, state.pressure);
  float springGain = 1.0f + 0.60f * spring;
  return envelope * modulator * springGain;
}
