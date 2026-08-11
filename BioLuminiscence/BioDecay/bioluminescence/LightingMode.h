// ============================================================
// LightingMode.h
// Abstract base class – mirrors the original lighting-mode project
// structure but adapted for time-based lighting behaviour.
// Every concrete lighting mode inherits from this.
// ============================================================
#ifndef LIGHTING_MODE_H
#define LIGHTING_MODE_H

#include "ProjectConfig.h"

struct TouchEvent {
  uint8_t panelId;
  uint8_t xCell;
  uint8_t yCell;
  bool    isTouched;
  int16_t pressure;
};

class LightingMode {
public:
  virtual ~LightingMode() {}

  virtual void enter(Adafruit_NeoPixel& strip) = 0;

  virtual void onTouch(Adafruit_NeoPixel& strip,
                       const TouchEvent& event) = 0;

  virtual void update(Adafruit_NeoPixel& strip) = 0;

  virtual const char* getName() = 0;

  static uint8_t intensityToBrightness(float I) {
    if (I <= 0.0f) return 0;
    if (I >= 120.0f) return 120;
    return (uint8_t)I;
  }

  static float massSpringResponse(float t_ms,
                                  int16_t pressure,
                                  float m = 1.0f,
                                  float c = 3.6f,
                                  float k = 25.0f,
                                  float forceGain = 8.0f) {
    if (t_ms <= 0.0f) return 0.0f;

    float t = t_ms * 0.001f;
    float p = constrain((float)pressure / 255.0f, 0.0f, 1.0f);
    float J = (0.05f + p) * forceGain;

    float disc = c * c - 4.0f * m * k;
    float x = 0.0f;

    if (disc < -1e-6f) {
      float wd = sqrtf(4.0f * m * k - c * c) / (2.0f * m);
      x = (J / (m * wd)) * expf(-(c / (2.0f * m)) * t) * sinf(wd * t);
    } else if (fabsf(disc) <= 1e-6f) {
      float r = -c / (2.0f * m);
      x = (J / m) * t * expf(r * t);
    } else {
      float sqrtDisc = sqrtf(disc);
      float r1 = (-c + sqrtDisc) / (2.0f * m);
      float r2 = (-c - sqrtDisc) / (2.0f * m);
      if (fabsf(r2 - r1) > 1e-6f) {
        x = (J / (m * (r2 - r1))) * (expf(r1 * t) - expf(r2 * t));
      }
    }

    if (x < 0.0f) x = 0.0f;
    return constrain(x, 0.0f, 1.4f);
  }
};

#endif // LIGHTING_MODE_H