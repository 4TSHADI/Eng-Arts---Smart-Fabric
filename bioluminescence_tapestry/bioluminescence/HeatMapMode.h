// ============================================================
// HeatMapMode.h
// MODEL – Touch-Accumulating Heat Map
//
// Each electrode stores a persistent "heat" level in [0.0, 1.0].
//   • Every touch INCREASES the heat for that pin (accumulates).
//   • When not being touched, heat DECAYS slowly toward 0.
//   • Heat  1.0 → red   (most-touched / hottest)
//   • Heat  0.5 → yellow / orange
//   • Heat  0.0 → green  (least-touched / coolest)
//
// Spatial blending: each pin's Gaussian region spills onto
// adjacent pixels, so a hot zone transitions smoothly into
// cooler neighbours — exactly like a thermal heat map.
// ============================================================
#ifndef HEAT_MAP_MODE_H
#define HEAT_MAP_MODE_H

#include "DecayMode.h"

// ── Per-pin heat state ───────────────────────────────────────
struct PinHeat {
  float         heat;        // accumulated heat level  [0.0 – 1.0]
  float         brightness;  // current peak brightness [0 – 255]
  bool          isTouched;   // finger currently down?
  unsigned long lastTouch;   // millis() of last touch event
};

class HeatMapMode : public DecayMode {
public:
  // ── Tuneable parameters ────────────────────────────────────
  // How much heat a single touch adds (per call to onTouch).
  float heatPerTouch;   // default 0.18  → ~6 touches to saturate

  // How quickly heat drains when the pin is idle.
  // heat -= coolRate * dt_s  each update tick.
  float coolRate;       // default 0.04  → full cool ~25 s of idling

  // Minimum brightness kept on every pixel (dim ambient glow).
  float ambientBri;     // default 4.0

  // Tau of the per-touch brightness flash [ms]
  // — gives a quick blink on touch even if heat hasn't built up yet.
  float flashTau;       // default 400 ms

  HeatMapMode()
    : heatPerTouch(0.18f),
      coolRate(0.04f),
      ambientBri(4.0f),
      flashTau(400.0f) {}

  // ── Lifecycle ──────────────────────────────────────────────
  void enter  (Adafruit_NeoPixel& strip) override;
  void onTouch(Adafruit_NeoPixel& strip,
               const TouchEvent& event) override;
  void update (Adafruit_NeoPixel& strip) override;
  const char* getName() override { return "Heat Map  (touch -> red, cool -> green)"; }

private:
  PinHeat      _pins[NUM_PINS];
  unsigned long _lastUpdate;   // millis() of previous update() call

  // Render the full heat map onto the strip buffer (no show()).
  void renderFrame(Adafruit_NeoPixel& strip);

  // Map heat [0,1] → RGB using a green→yellow→orange→red palette.
  // Returns a packed NeoPixel colour.
  uint32_t heatColor(Adafruit_NeoPixel& strip, float heat, float brightness);
};

#endif // HEAT_MAP_MODE_H
