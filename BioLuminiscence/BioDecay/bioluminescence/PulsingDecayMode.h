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

#include "DecayMode.h"

class PulsingDecayMode : public DecayMode {
public:
  // ── Parameters (tuneable) ──────────────────────────────────
  float I0;    // Peak intensity     [0-255]        default 255
  float tau;   // Decay time const   [milliseconds] default 1200
  float a;     // Sine amplitude     [0.0-1.0]      default 0.4
  float omega; // Angular frequency  [rad/ms]       default ~0.006
               //   → 1 Hz pulse ≈ 2π/1000 ≈ 0.00628

  PulsingDecayMode()
    : I0(255.0f), tau(1200.0f), a(0.4f), omega(0.00628f) {}

  // ── Lifecycle ──────────────────────────────────────────────
  void enter(Adafruit_NeoPixel& strip) override;
  void onTouch(Adafruit_NeoPixel& strip,
               uint8_t pin, bool isTouched, int16_t pressure) override;
  void update(Adafruit_NeoPixel& strip) override;
  const char* getName() override {
    return "Pulsing Decay  I(t)=I0*exp(-t/tau)*(1+a*sin(wt))";
  }

private:
  bool          _active;
  unsigned long _startTime;
  uint8_t       _triggeredPin;
  float         _I0_actual;

  float computeIntensity(float t_ms);
};

#endif // PULSING_DECAY_MODE_H
