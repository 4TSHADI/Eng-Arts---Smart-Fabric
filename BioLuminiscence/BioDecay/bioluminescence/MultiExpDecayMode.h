// ============================================================
// MultiExpDecayMode.h
// MODEL 3 – Multi-exponential decay (dinoflagellates)
//
//   I(t) = A₁·e^(−t/τ₁) + A₂·e^(−t/τ₂)
//
// Organism analogue: dinoflagellates (e.g. Pyrocystis, Noctiluca),
// dinophytes, and many bioluminescent reactions that have:
//   • Component 1 (fast)  – direct luciferase emission  → small τ₁
//   • Component 2 (slow)  – afterglow / secondary fluor  → large τ₂
// ============================================================
#ifndef MULTI_EXP_DECAY_MODE_H
#define MULTI_EXP_DECAY_MODE_H

#include "SimpleDecayMode.h"

class MultiExpDecayMode : public SimpleDecayMode {
public:
  // ── Parameters (tuneable) ──────────────────────────────────
  float A1;   // Amplitude of fast component  [0-255] default 200
  float tau1; // Fast time constant  [ms]              default 300
  float A2;   // Amplitude of slow component  [0-255] default 80
  float tau2; // Slow time constant  [ms]              default 2000

  MultiExpDecayMode()
    : A1(200.0f), tau1(300.0f), A2(80.0f), tau2(2000.0f) {
      tau = tau1;
    }

  // ── Lifecycle ──────────────────────────────────────────────
  void enter(Adafruit_NeoPixel& strip) override;
  void onTouch(Adafruit_NeoPixel& strip,
               const TouchEvent& event) override;
  void update(Adafruit_NeoPixel& strip) override;
  const char* getName() override {
    return "Multi-Exp Decay  I(t)=A1*exp(-t/t1)+A2*exp(-t/t2)";
  }

private:
  static const uint8_t kBlobCount = 9;
  float         _scale;  // pressure-derived amplitude scale [0.0-1.0]

  float computeIntensity(const TouchDecayState& state, float t_ms) override;
  void renderBlobField(Adafruit_NeoPixel& strip,
                       uint16_t touchPoint,
                       const TouchDecayState& state,
                       float t_ms);
};

#endif // MULTI_EXP_DECAY_MODE_H
