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

#include "DecayMode.h"

class MultiExpDecayMode : public DecayMode {
public:
  // ── Parameters (tuneable) ──────────────────────────────────
  float A1;   // Amplitude of fast component  [0-255] default 200
  float tau1; // Fast time constant  [ms]              default 300
  float A2;   // Amplitude of slow component  [0-255] default 80
  float tau2; // Slow time constant  [ms]              default 2000

  MultiExpDecayMode()
    : A1(200.0f), tau1(300.0f), A2(80.0f), tau2(2000.0f) {}

  // ── Lifecycle ──────────────────────────────────────────────
  void enter(Adafruit_NeoPixel& strip) override;
  void onTouch(Adafruit_NeoPixel& strip,
               uint8_t pin, bool isTouched, int16_t pressure) override;
  void update(Adafruit_NeoPixel& strip) override;
  const char* getName() override {
    return "Multi-Exp Decay  I(t)=A1*exp(-t/t1)+A2*exp(-t/t2)";
  }

private:
  bool          _active;
  unsigned long _startTime;
  uint8_t       _triggeredPin;
  float         _scale;  // pressure-derived amplitude scale [0.0-1.0]

  float computeIntensity(float t_ms);
};

#endif // MULTI_EXP_DECAY_MODE_H
