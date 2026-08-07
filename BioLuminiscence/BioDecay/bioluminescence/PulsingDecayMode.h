// ============================================================
// PulsingDecayMode.h
// MODEL 2 – Pulsing / breathing bioluminescence
//
//   I(t) = I₀ · e^(−t/τ) · (1 + a·sin(ωt))
//
// Organism analogue: ctenophores, some jellyfish species that
// combine a decaying glow with a rhythmic "breathing" pulse.
// The sine term adds ±a fraction of peak intensity as a wave
// riding on top of the exponential envelope.
// ============================================================
#ifndef PULSING_DECAY_MODE_H
#define PULSING_DECAY_MODE_H

#include "SimpleDecayMode.h"

class PulsingDecayMode : public SimpleDecayMode {
public:
  // ── Parameters (tuneable) ──────────────────────────────────
  float a;     // Sine amplitude     [0.0-1.0]      default 0.4
  float omega; // Angular frequency  [rad/ms]       default ~0.006
               //   → 1 Hz pulse ≈ 2π/1000 ≈ 0.00628

  PulsingDecayMode()
    : a(0.4f), omega(0.00628f) {
      tau = 1200.0f;
    }

  // ── Lifecycle ──────────────────────────────────────────────
  void enter(Adafruit_NeoPixel& strip) override;
  const char* getName() override {
    return "Pulsing Decay  I(t)=I0*exp(-t/tau)*(1+a*sin(wt))";
  }

private:
  float computeIntensity(const TouchDecayState& state, float t_ms) override;
};

#endif // PULSING_DECAY_MODE_H
