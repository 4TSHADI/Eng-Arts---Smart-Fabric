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
  uint8_t axisId;
  uint8_t electrode;
  uint8_t globalPin;
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
  // The event carries panel, axis, and electrode identity plus pressure.
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
};

#endif // DECAY_MODE_H
