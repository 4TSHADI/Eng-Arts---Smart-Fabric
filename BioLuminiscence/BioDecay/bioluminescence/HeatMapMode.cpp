// ============================================================
// HeatMapMode.cpp
// MODEL – Touch-Accumulating Heat Map
//
// Behaviour summary
// ─────────────────
//  • Each touch point maintains a persistent "heat" value H ∈ [0,1].
//  • Touch  → H increases by heatPerTouch (clamped to 1.0).
//  • Idle   → H decreases at coolRate units·s⁻¹.
//  • Render → heat mapped through blue→cyan→amber→red palette,
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

  for (uint16_t p = 0; p < NUM_TOUCH_POINTS; p++) {
    _touchStates[p].heat       = 0.0f;
    _touchStates[p].brightness = 0.0f;
    _touchStates[p].isTouched  = false;
    _touchStates[p].pressure   = 0;
    _touchStates[p].lastTouch  = 0;
  }

  if (MODE_EVENT_SERIAL_LOG) {
    Serial.println("[HeatMap] Entered – touch coordinates to build heat.");
    Serial.println("          Red = hot (often touched), Blue = cool (resting).");
  }
}

// ─────────────────────────────────────────────────────────────

void HeatMapMode::onTouch(Adafruit_NeoPixel& strip,
                           const TouchEvent& event) {
  if (event.xCell >= TOUCH_GRID_SIZE || event.yCell >= TOUCH_GRID_SIZE) return;

  uint16_t touchPoint = touchPointIndex(event.panelId, event.xCell, event.yCell);

  _touchStates[touchPoint].isTouched = event.isTouched;

  if (event.isTouched) {
    _touchStates[touchPoint].pressure = event.pressure;
    float springBoost = massSpringResponse(100.0f, event.pressure);
    float boost = heatPerTouch * constrain(map(event.pressure, 0, 255, 80, 255), 80, 255) / 255.0f;
    boost *= (1.0f + 0.45f * springBoost);
    _touchStates[touchPoint].heat = min(1.0f, _touchStates[touchPoint].heat + boost);

    _touchStates[touchPoint].brightness  = constrain(map(event.pressure, 0, 255, 150, 255) * (1.0f + 0.35f * springBoost), 150, 255);
    _touchStates[touchPoint].lastTouch   = millis();

    if (MODE_EVENT_SERIAL_LOG) {
      Serial.print("[HeatMap] panel="); Serial.print(event.panelId);
      Serial.print(" x="); Serial.print(event.xCell);
      Serial.print(" y="); Serial.print(event.yCell);
      Serial.print(" pressure="); Serial.print(event.pressure);
      Serial.print(" heat="); Serial.println(_touchStates[touchPoint].heat, 3);
    }
  }
}

// ─────────────────────────────────────────────────────────────

void HeatMapMode::update(Adafruit_NeoPixel& strip) {
  unsigned long now = millis();
  float dt_s = (now - _lastUpdate) / 1000.0f;
  _lastUpdate = now;

  // Cap dt in case the system stalls (e.g. first tick, mode switch).
  if (dt_s > 0.5f) dt_s = 0.5f;

  bool anyVisible = false;

  for (uint16_t p = 0; p < NUM_TOUCH_POINTS; p++) {
    if (_touchStates[p].isTouched) {
      float tTouchMs = (float)(now - _touchStates[p].lastTouch);
      float springHold = massSpringResponse(tTouchMs, _touchStates[p].pressure);
      _touchStates[p].heat += holdHeatRate * (1.0f + 0.35f * springHold) * dt_s;
      if (_touchStates[p].heat > 1.0f) _touchStates[p].heat = 1.0f;
    } else {
      _touchStates[p].heat -= coolRate * dt_s;
      if (_touchStates[p].heat < 0.0f) _touchStates[p].heat = 0.0f;
    }

    float t_ms   = (float)(now - _touchStates[p].lastTouch);
    float flashI = _touchStates[p].brightness * expf(-t_ms / flashTau);

    float heatFloor = ambientBri + _touchStates[p].heat * 220.0f;
    float bri = max(flashI, heatFloor);
    _touchStates[p].brightness = (bri > flashI) ? bri : flashI;

    if (_touchStates[p].heat > 0.002f || flashI > 1.0f) anyVisible = true;
  }

  if (!anyVisible) return;

  strip.clear();
  renderFrame(strip);
  strip.show();
}

// ── Rendering ────────────────────────────────────────────────

void HeatMapMode::renderFrame(Adafruit_NeoPixel& strip) {
  unsigned long now = millis();

  for (uint16_t p = 0; p < NUM_TOUCH_POINTS; p++) {
    float t_ms   = (float)(now - _touchStates[p].lastTouch);
    float flashI = _touchStates[p].brightness * expf(-t_ms / flashTau);
    float heatFloor = ambientBri + _touchStates[p].heat * 220.0f;
    float bri    = max(flashI, heatFloor);

    if (bri < 1.0f && _touchStates[p].heat < 0.002f) continue;

    PinRegion region = getTouchRegion(touchPointPanel(p), touchPointX(p), touchPointY(p));
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

        uint32_t newColor = heatColor(strip, _touchStates[p].heat,
                                       constrain(scaledBri, 0.0f, 255.0f));

        uint32_t existing = strip.getPixelColor(idx);
        uint8_t eR = (existing >> 16) & 0xFF;
        uint8_t eG = (existing >>  8) & 0xFF;
        uint8_t eB = (existing)       & 0xFF;
        uint8_t nR = (newColor  >> 16) & 0xFF;
        uint8_t nG = (newColor  >>  8) & 0xFF;
        uint8_t nB = (newColor)        & 0xFF;

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
// Thermal ramp:  blue → cyan → amber → red
//
//   heat 0.00 → R=  0, G= 20, B=220  (cool blue)
//   heat 0.33 → R=  0, G=200, B=255  (cyan)
//   heat 0.66 → R=255, G=140, B=  0  (amber)
//   heat 1.00 → R=255, G=  0, B=  0  (hot red)
//
// brightness scales all channels proportionally [0..255].

uint32_t HeatMapMode::heatColor(Adafruit_NeoPixel& strip,
                                 float heat, float brightness) {
  heat = constrain(heat, 0.0f, 1.0f);
  float scale = brightness / 255.0f;

  float r, g, b;

  if (heat < 0.33f) {
    // Blue -> Cyan
    float t = heat / 0.33f;
    r = 0.0f;
    g = 20.0f + 180.0f * t;
    b = 220.0f + 35.0f * t;
  } else if (heat < 0.66f) {
    // Cyan -> Amber
    float t = (heat - 0.33f) / 0.33f;
    r = 255.0f * t;
    g = 200.0f - 60.0f * t;
    b = 255.0f - 255.0f * t;
  } else {
    // Amber -> Red
    float t = (heat - 0.66f) / 0.34f;
    r = 255.0f;
    g = 140.0f - 140.0f * t;
    b = 0.0f;
  }

  return strip.Color(
    (uint8_t)constrain(r * scale, 0.0f, 255.0f),
    (uint8_t)constrain(g * scale, 0.0f, 255.0f),
    (uint8_t)constrain(b * scale, 0.0f, 255.0f)
  );
}
