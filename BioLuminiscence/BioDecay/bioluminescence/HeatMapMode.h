// ============================================================
// HeatMapMode.h
// MODEL – Touch-Accumulating Heat Map
//
// Each touch point stores a persistent "heat" level in [0.0, 1.0].
//   • Every touch INCREASES the heat for that cell (accumulates).
//   • When not being touched, heat fades slowly toward 0.
//   • Heat  1.0 → red    (most-touched / hottest)
//   • Heat  0.5 → cyan / amber transition
//   • Heat  0.0 → blue   (least-touched / coolest)
//
// Spatial blending: each touch point's Gaussian region spills onto
// adjacent pixels, so a hot zone transitions smoothly into
// cooler neighbours — exactly like a thermal heat map.
// ============================================================
#ifndef HEAT_MAP_MODE_H
#define HEAT_MAP_MODE_H

#include "LightingMode.h"

// ── Per-touch-point heat state ───────────────────────────────
struct TouchHeatState {
  float         heat;        // accumulated heat level  [0.0 – 1.0]
  float         brightness;  // current peak brightness [0 – 255]
  bool          isTouched;   // finger currently down?
  int16_t       pressure;    // last touch pressure for MSD dynamics
  unsigned long lastTouch;   // millis() of last touch event
};

class HeatMapMode : public LightingMode {
public:
  // ── Tuneable parameters ────────────────────────────────────
  // How much heat a single touch adds (per call to onTouch).
  float heatPerTouch;   // default 0.18  → ~6 touches to saturate

  // Additional heat gain while a touch is continuously held.
  // heat += holdHeatRate * dt_s each update tick while touched.
  float holdHeatRate;   // default 0.10

  // How quickly heat drains when the pin is idle.
  // heat -= coolRate * dt_s  each update tick.
  float coolRate;       // default 0.04  → full cool ~25 s of idling

  // Minimum brightness kept on every pixel (dim ambient glow).
  float ambientBri;     // default 4.0

  // Tau of the per-touch brightness flash [ms]
  // — gives a quick blink on touch even if heat hasn't built up yet.
  float flashTau;       // default 400 ms

  HeatMapMode()
    : heatPerTouch(0.30f),
      holdHeatRate(0.20f),
      coolRate(0.04f),
      ambientBri(10.0f),
      flashTau(400.0f) {}

  // ── Lifecycle ──────────────────────────────────────────────
  void enter  (Adafruit_NeoPixel& strip) override;
  void onTouch(Adafruit_NeoPixel& strip,
               const TouchEvent& event) override;
  void update (Adafruit_NeoPixel& strip) override;
  const char* getName() override { return "Heat Map  (touch -> red, cool -> blue)"; }

private:
  TouchHeatState _touchStates[NUM_TOUCH_POINTS];
  unsigned long _lastUpdate;   // millis() of previous update() call

  // Render the full heat map onto the strip buffer (no show()).
  void renderFrame(Adafruit_NeoPixel& strip);

  // Map heat [0,1] → RGB using a blue→cyan→amber→red palette.
  // Returns a packed NeoPixel colour.
  uint32_t heatColor(Adafruit_NeoPixel& strip, float heat, float brightness);
};

#endif // HEAT_MAP_MODE_H
