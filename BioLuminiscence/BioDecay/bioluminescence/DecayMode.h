// ============================================================
// DecayMode.h
// Abstract base class – mirrors LightingMode.h from the
// mpr121_multi_mode project but adapted for time-based decay.
// Every concrete decay model inherits from this.
// ============================================================
#ifndef DECAY_MODE_H
#define DECAY_MODE_H

#include "ProjectConfig.h"

struct TouchEvent {
  uint8_t panelId;
  uint8_t xCell;
  uint8_t yCell;
  bool    isTouched;
  int16_t pressure;
};

class DecayMode {
public:
  virtual ~DecayMode() {}

  // ── Lifecycle ──────────────────────────────────────────────

  // Called once when the mode becomes active.
  // Reset all internal state here.
  virtual void enter(Adafruit_NeoPixel& strip) = 0;

  // Called when a touch / release event fires.
  //   panelId   : LED panel index driven by this X/Y sensor pair
  //   xCell/yCell: touched coordinate on the 8x8 touch grid
  //   isTouched : true = finger down, false = finger lifted
  //   pressure  : baseline - filtered (proxy for touch strength)
  virtual void onTouch(Adafruit_NeoPixel& strip,
                       const TouchEvent& event) = 0;

  // Called every loop() cycle for continuous animation updates.
  virtual void update(Adafruit_NeoPixel& strip) = 0;

  // Human-readable label shown in Serial Monitor.
  virtual const char* getName() = 0;

  // ── Shared math helpers ────────────────────────────────────
  // Evaluate any decay I(t) and clamp result to [0, 255].
  // Subclasses call this to convert float intensity → LED brightness.
  static uint8_t intensityToBrightness(float I) {
    if (I <= 0.0f) return 0;
    if (I >= 120.0f) return 120;
    return (uint8_t)I;
  }

  // Mass-spring-damper impulse response driven by touch pressure.
  // Equation: m*x'' + c*x' + k*x = F(t), with F(t)=J*delta(t).
  // t_ms      : elapsed time since touch [ms]
  // pressure  : touch strength proxy from SensorHub
  // m, c, k   : mass, damping, stiffness model parameters
  // forceGain : scales pressure into impulse magnitude J
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

#endif // DECAY_MODE_H
