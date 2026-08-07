// ============================================================
// SimpleDecayMode.h
// MODEL 1 – First-order reaction kinetics
//
//   I(t) = I₀ · e^(−t/τ)
//
// Each touch point has its own independent decay so multiple cells
// can glow at the same time.  The glow of each region is brightest
// at its centre and falls off with a Gaussian profile.
// ============================================================
#ifndef SIMPLE_DECAY_MODE_H
#define SIMPLE_DECAY_MODE_H

#include "DecayMode.h"

// ── Per-touch decay state ────────────────────────────────────
struct TouchDecayState {
  bool          active;      // Is a decay running on this touch point?
  unsigned long startTime;   // millis() when it was triggered
  float         I0_actual;   // Peak intensity for this trigger [0-255]
  int16_t       pressure;    // Touch impulse strength
};

class SimpleDecayMode : public DecayMode {
public:
  // ── Parameters (tuneable) ──────────────────────────────────
  float I0;   // Default peak intensity [0-255]  default 255
  float tau;  // Time constant          [ms]     default 800

  SimpleDecayMode() : I0(255.0f), tau(800.0f) {}

  // ── Lifecycle ──────────────────────────────────────────────
  void enter(Adafruit_NeoPixel& strip) override;
  void onTouch(Adafruit_NeoPixel& strip,
               const TouchEvent& event) override;
  void update(Adafruit_NeoPixel& strip) override;
  const char* getName() override { return "Simple Decay  I(t)=I0*exp(-t/tau)"; }

protected:
  TouchDecayState _touchStates[NUM_TOUCH_POINTS];

  // Evaluate I(t) for one touch-point decay
  virtual float computeIntensity(const TouchDecayState& state, float t_ms);

private:
  // Render all active touch-point glows onto the strip buffer (no show())
  void renderFrame(Adafruit_NeoPixel& strip);
};

#endif // SIMPLE_DECAY_MODE_H
