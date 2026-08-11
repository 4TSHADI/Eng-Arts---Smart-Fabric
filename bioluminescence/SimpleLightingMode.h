// ============================================================
// SimpleLightingMode.h
// MODEL 1 – First-order reaction kinetics
//
//   I(t) = I₀ · e^(−t/τ)
//
// Each touch point has its own independent lighting response so multiple cells
// can glow at the same time.  The glow of each region is brightest
// at its centre and falls off with a Gaussian profile.
// ============================================================
#ifndef SIMPLE_LIGHTING_MODE_H
#define SIMPLE_LIGHTING_MODE_H

#include "LightingMode.h"

struct TouchLightingState {
  bool          active;
  unsigned long startTime;
  float         I0_actual;
  int16_t       pressure;
};

class SimpleLightingMode : public LightingMode {
public:
  float I0;
  float tau;

  SimpleLightingMode() : I0(255.0f), tau(800.0f) {}

  void enter(Adafruit_NeoPixel& strip) override;
  void onTouch(Adafruit_NeoPixel& strip,
               const TouchEvent& event) override;
  void update(Adafruit_NeoPixel& strip) override;
  const char* getName() override { return "Simple Lighting  I(t)=I0*exp(-t/tau)"; }

protected:
  TouchLightingState _touchStates[NUM_TOUCH_POINTS];
  virtual float computeIntensity(const TouchLightingState& state, float t_ms);

private:
  void renderFrame(Adafruit_NeoPixel& strip);
};

#endif // SIMPLE_LIGHTING_MODE_H