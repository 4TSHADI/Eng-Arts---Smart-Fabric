// ============================================================
// MultiExpLightingMode.h
// MODEL 3 – Multi-exponential lighting response (dinoflagellates)
//
//   I(t) = A₁·e^(−t/τ₁) + A₂·e^(−t/τ₂)
// ============================================================
#ifndef MULTI_EXP_LIGHTING_MODE_H
#define MULTI_EXP_LIGHTING_MODE_H

#include "SimpleLightingMode.h"

class MultiExpLightingMode : public SimpleLightingMode {
public:
  float A1;
  float tau1;
  float A2;
  float tau2;

  MultiExpLightingMode()
    : A1(200.0f), tau1(300.0f), A2(80.0f), tau2(2000.0f) {
      tau = tau1;
    }

  void enter(Adafruit_NeoPixel& strip) override;
  void onTouch(Adafruit_NeoPixel& strip,
               const TouchEvent& event) override;
  void update(Adafruit_NeoPixel& strip) override;
  const char* getName() override {
    return "Multi-Exp Lighting  I(t)=A1*exp(-t/t1)+A2*exp(-t/t2)";
  }

private:
  static const uint8_t kBlobCount = 9;
  float         _scale;

  float computeIntensity(const TouchLightingState& state, float t_ms) override;
  void renderBlobField(Adafruit_NeoPixel& strip,
                       uint16_t touchPoint,
                       const TouchLightingState& state,
                       float t_ms);
};

#endif // MULTI_EXP_LIGHTING_MODE_H