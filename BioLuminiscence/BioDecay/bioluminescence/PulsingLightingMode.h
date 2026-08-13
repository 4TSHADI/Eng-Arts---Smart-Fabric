// ============================================================
// PulsingLightingMode.h
// MODEL 2 – Pulsing / breathing bioluminescence
//
//   I(t) = I₀ · e^(−t/τ) · (1 + a·sin(ωt))
//
// Organism analogue: ctenophores, some jellyfish species that
// combine a fading glow with a rhythmic "breathing" pulse.
// ============================================================
#ifndef PULSING_LIGHTING_MODE_H
#define PULSING_LIGHTING_MODE_H

#include "SimpleLightingMode.h"

class PulsingLightingMode : public SimpleLightingMode {
public:
  float a;
  float omega;

  PulsingLightingMode()
    : a(0.4f), omega(0.00628f) {
      tau = 1200.0f;
    }

  void enter(Adafruit_NeoPixel& strip) override;
  const char* getName() override {
    return "Pulsing Lighting  I(t)=I0*exp(-t/tau)*(1+a*sin(wt))";
  }

private:
  float computeIntensity(const TouchLightingState& state, float t_ms) override;
};

#endif // PULSING_LIGHTING_MODE_H