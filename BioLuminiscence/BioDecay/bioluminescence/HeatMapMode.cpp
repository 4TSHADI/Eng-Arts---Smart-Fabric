// ============================================================
// HeatMapMode.cpp
// MODEL – Touch-Accumulating Heat Map
//
// Behaviour summary
// ─────────────────
//  • Each electrode maintains a persistent "heat" value H ∈ [0,1].
//  • Touch  → H increases by heatPerTouch (clamped to 1.0).
//  • Idle   → H decreases at coolRate units·s⁻¹.
//  • Render → heat mapped through green→yellow→orange→red palette,
//             painted with a Gaussian gradient so the glow is
//             brightest at the region centre and fades outward.
//  • Neighbouring regions share pixels via overlapping Gaussians,
//    so a "hot" zone beside a "cool" zone blends smoothly —
//    matching the heat-map aesthetic in the reference image.
// ============================================================
#include "HeatMapMode.h"

// ── Lifecycle ─────────────────────────────────────────────────

void HeatMapMode::enter(Adafruit_NeoPixel& strip) {
  strip.clear();
  strip.show();

  _lastUpdate = millis();

  for (uint8_t p = 0; p < NUM_PINS; p++) {
    _pins[p].heat       = 0.0f;
    _pins[p].brightness = 0.0f;
    _pins[p].isTouched  = false;
    _pins[p].lastTouch  = 0;
  }

  Serial.println("[HeatMap] Entered – touch electrodes to build heat.");
  Serial.println("          Red = hot (often touched), Green = cool (resting).");
}

// ─────────────────────────────────────────────────────────────

void HeatMapMode::onTouch(Adafruit_NeoPixel& strip,
                           uint8_t pin, bool isTouched, int16_t pressure) {
  if (pin >= NUM_PINS) return;

  _pins[pin].isTouched = isTouched;

  if (isTouched) {
    // Accumulate heat – heavier pressure = bigger boost (optional flavour).
    float boost = heatPerTouch * constrain(map(pressure, 0, 150, 80, 150), 80, 150) / 150.0f;
    _pins[pin].heat = min(1.0f, _pins[pin].heat + boost);

    // Also give a bright flash so the touch is immediately visible.
    _pins[pin].brightness  = constrain(map(pressure, 0, 150, 120, 255), 120, 255);
    _pins[pin].lastTouch   = millis();

    Serial.print("[HeatMap] pin="); Serial.print(pin);
    Serial.print("  pressure=");    Serial.print(pressure);
    Serial.print("  heat=");        Serial.println(_pins[pin].heat, 3);
  }
  // Finger-up: let heat decay naturally — nothing extra needed.
}

// ─────────────────────────────────────────────────────────────

void HeatMapMode::update(Adafruit_NeoPixel& strip) {
  unsigned long now = millis();
  float dt_s = (now - _lastUpdate) / 1000.0f;
  _lastUpdate = now;

  // Cap dt in case the system stalls (e.g. first tick, mode switch).
  if (dt_s > 0.5f) dt_s = 0.5f;

  bool anyVisible = false;

  for (uint8_t p = 0; p < NUM_PINS; p++) {
    // ── 1. Cool idle pins ─────────────────────────────────
    if (!_pins[p].isTouched) {
      _pins[p].heat -= coolRate * dt_s;
      if (_pins[p].heat < 0.0f) _pins[p].heat = 0.0f;
    }

    // ── 2. Decay the flash brightness ─────────────────────
    float t_ms   = (float)(now - _pins[p].lastTouch);
    float flashI = _pins[p].brightness * expf(-t_ms / flashTau);

    // Floor brightness from heat so that even a cool (green) zone
    // has a faint glow once it has been touched at least a little.
    float heatFloor = ambientBri + _pins[p].heat * 180.0f;

    // Final per-pin brightness = max of flash tail and heat floor.
    float bri = max(flashI, heatFloor);

    // Store computed brightness for renderFrame.
    _pins[p].brightness = (bri > flashI) ? bri : flashI;

    if (_pins[p].heat > 0.002f || flashI > 1.0f) anyVisible = true;
  }

  if (!anyVisible) return; // nothing to draw

  strip.clear();
  renderFrame(strip);
  strip.show();
}

// ── Rendering ────────────────────────────────────────────────

void HeatMapMode::renderFrame(Adafruit_NeoPixel& strip) {
  unsigned long now = millis();

  for (uint8_t p = 0; p < NUM_PINS; p++) {
    // Compute effective brightness for this pin this frame.
    float t_ms   = (float)(now - _pins[p].lastTouch);
    float flashI = _pins[p].brightness * expf(-t_ms / flashTau);
    float heatFloor = ambientBri + _pins[p].heat * 180.0f;
    float bri    = max(flashI, heatFloor);

    if (bri < 1.0f && _pins[p].heat < 0.002f) continue;

    // Broaden sigma slightly for a softer glow edge.
    PinRegion region = getPinRegion(p);
    const float spreadX = region.sigmaX * 1.25f;
    const float spreadY = region.sigmaY * 1.25f;

    int xMin = max(0, (int)(region.cx - 4.0f * spreadX));
    int xMax = min((int)WIDTH  - 1, (int)(region.cx + 4.0f * spreadX));
    int yMin = max(0, (int)(region.cy - 4.0f * spreadY));
    int yMax = min((int)HEIGHT - 1, (int)(region.cy + 4.0f * spreadY));

    for (int y = yMin; y <= yMax; y++) {
      for (int x = xMin; x <= xMax; x++) {
        float dx  = (float)x - region.cx;
        float dy  = (float)y - region.cy;
        float gau = gaussianBrightness(dx, dy, spreadX, spreadY);
        gau = powf(gau, 0.85f); // soften falloff

        float scaledBri = bri * gau;
        if (scaledBri < 1.0f) continue;

        uint16_t idx = XY((uint8_t)x, (uint8_t)y);
        if (idx >= NUM_LEDS) continue;

        uint32_t newColor = heatColor(strip, _pins[p].heat,
                                       constrain(scaledBri, 0.0f, 255.0f));

        // Blend with whatever is already on this pixel (overlapping regions).
        uint32_t existing = strip.getPixelColor(idx);
        uint8_t eR = (existing >> 16) & 0xFF;
        uint8_t eG = (existing >>  8) & 0xFF;
        uint8_t eB = (existing)       & 0xFF;
        uint8_t nR = (newColor  >> 16) & 0xFF;
        uint8_t nG = (newColor  >>  8) & 0xFF;
        uint8_t nB = (newColor)        & 0xFF;

        // Soft-max blend: dominant channel wins, minority adds a hint.
        auto softBlend = [](uint8_t a, uint8_t b) -> uint8_t {
          return (uint8_t)min(255u,
                              (uint16_t)max(a, b) + ((uint16_t)min(a, b) >> 2));
        };

        strip.setPixelColor(idx, strip.Color(
          softBlend(eR, nR),
          softBlend(eG, nG),
          softBlend(eB, nB)
        ));
      }
    }
  }
}

// ── Colour palette ────────────────────────────────────────────
//
// Thermal ramp:  green → yellow → orange → red
//
//   heat 0.00 → R=  0, G=220, B=  0  (cool green)
//   heat 0.33 → R=220, G=220, B=  0  (yellow)
//   heat 0.66 → R=255, G= 80, B=  0  (orange)
//   heat 1.00 → R=255, G=  0, B=  0  (hot red)
//
// brightness scales all channels proportionally [0..255].

uint32_t HeatMapMode::heatColor(Adafruit_NeoPixel& strip,
                                 float heat, float brightness) {
  heat = constrain(heat, 0.0f, 1.0f);
  float scale = brightness / 255.0f;

  float r, g, b;

  if (heat < 0.33f) {
    // Green → Yellow
    float t = heat / 0.33f;
    r = 220.0f * t;
    g = 220.0f;
    b = 0.0f;
  } else if (heat < 0.66f) {
    // Yellow → Orange
    float t = (heat - 0.33f) / 0.33f;
    r = 220.0f + 35.0f * t;
    g = 220.0f - 140.0f * t;
    b = 0.0f;
  } else {
    // Orange → Red
    float t = (heat - 0.66f) / 0.34f;
    r = 255.0f;
    g = 80.0f - 80.0f * t;
    b = 0.0f;
  }

  return strip.Color(
    (uint8_t)constrain(r * scale, 0.0f, 255.0f),
    (uint8_t)constrain(g * scale, 0.0f, 255.0f),
    (uint8_t)constrain(b * scale, 0.0f, 255.0f)
  );
}
