// ============================================================
// SimpleDecayMode.cpp
// MODEL 1 – I(t) = I₀ · e^(−t/τ)
//
// Per-touch-point behaviour:
//   • Touch  → start an independent exponential decay for that cell
//   • update() → for every active cell, evaluate I(t) and paint its
//                region onto the frame buffer with a Gaussian
//                brightness gradient (bright centre, dark edges)
//   • Multiple touch points can glow simultaneously
// ============================================================
#include "SimpleDecayMode.h"

// ── Lifecycle ─────────────────────────────────────────────────

void SimpleDecayMode::enter(Adafruit_NeoPixel& strip) {
  strip.clear();
  strip.show();

  for (uint16_t p = 0; p < NUM_TOUCH_POINTS; p++) {
    _touchStates[p].active    = false;
    _touchStates[p].startTime = 0;
    _touchStates[p].I0_actual = 0.0f;
    _touchStates[p].pressure  = 0;
  }

  Serial.println("[SimpleDecay] Entered – touch coordinates trigger compact regions.");
}

void SimpleDecayMode::onTouch(Adafruit_NeoPixel& strip,
                               const TouchEvent& event) {
  if (event.xCell >= TOUCH_GRID_SIZE || event.yCell >= TOUCH_GRID_SIZE) return;

  uint16_t touchPoint = touchPointIndex(event.panelId, event.xCell, event.yCell);

  if (event.isTouched) {
    _touchStates[touchPoint].I0_actual = constrain(map(event.pressure, 0, 255, 80, 255), 80, 255);
    _touchStates[touchPoint].pressure  = event.pressure;
    _touchStates[touchPoint].startTime = millis();
    _touchStates[touchPoint].active    = true;

    Serial.print("[SimpleDecay] panel="); Serial.print(event.panelId);
    Serial.print(" x="); Serial.print(event.xCell);
    Serial.print(" y="); Serial.print(event.yCell);
    Serial.print(" pressure="); Serial.print(event.pressure);
    Serial.print(" I0="); Serial.println(_touchStates[touchPoint].I0_actual);
  }
}

void SimpleDecayMode::update(Adafruit_NeoPixel& strip) {
  bool anyActive = false;
  for (uint16_t p = 0; p < NUM_TOUCH_POINTS; p++) {
    if (_touchStates[p].active) { anyActive = true; break; }
  }
  if (!anyActive) return;

  strip.clear();
  renderFrame(strip);
  strip.show();
}

// ── Rendering ────────────────────────────────────────────────

void SimpleDecayMode::renderFrame(Adafruit_NeoPixel& strip) {
  unsigned long now = millis();

  for (uint16_t p = 0; p < NUM_TOUCH_POINTS; p++) {
    if (!_touchStates[p].active) continue;

    float t_ms = (float)(now - _touchStates[p].startTime);
    float I    = computeIntensity(_touchStates[p], t_ms);

    if (I < 1.0f) {
      _touchStates[p].active = false;
      Serial.print("[SimpleDecay] touchPoint="); Serial.print(p);
      Serial.println(" decay complete.");
      continue;
    }

    PinRegion region = getTouchRegion(touchPointPanel(p), touchPointX(p), touchPointY(p));
    // Pressure controls circle size: low pressure = tighter glow,
    // high pressure = larger glow footprint.
    float pressureNorm = constrain((float)_touchStates[p].pressure / 255.0f, 0.0f, 1.0f);
    float radiusScale = 0.25f + 1.35f * pressureNorm;
    const float spreadX = region.sigmaX * 1.25f * radiusScale;
    const float spreadY = region.sigmaY * 1.25f * radiusScale;

    int xMin = max(0, (int)(region.cx - 4.0f * spreadX));
    int xMax = min((int)WIDTH  - 1, (int)(region.cx + 4.0f * spreadX));
    int yMin = max(0, (int)(region.cy - 4.0f * spreadY));
    int yMax = min((int)HEIGHT - 1, (int)(region.cy + 4.0f * spreadY));

    for (int y = yMin; y <= yMax; y++) {
      for (int x = xMin; x <= xMax; x++) {
        float dx  = (float)x - region.cx;
        float dy  = (float)y - region.cy;
        float gau = gaussianBrightness(dx, dy, spreadX, spreadY);
        gau = powf(gau, 0.85f); // soften the falloff for a cleaner edge

        float scaledI = I * gau;
        if (scaledI < 1.0f) continue;

        uint8_t bri = (uint8_t)constrain(scaledI, 0.0f, 255.0f);

        uint16_t idx = XY((uint8_t)x, (uint8_t)y);
        if (idx >= NUM_LEDS) continue;

        uint32_t existing = strip.getPixelColor(idx);
        uint32_t newColor  = bioColor(strip, bri);

        uint8_t eG = (existing >> 8)  & 0xFF;
        uint8_t eB = (existing)       & 0xFF;
        uint8_t nG = (newColor  >> 8) & 0xFF;
        uint8_t nB = (newColor)       & 0xFF;

        uint8_t blendedG = (uint8_t)min(255u,
                                         (uint16_t)max(eG, nG) + ((uint16_t)min(eG, nG) / 4u));
        uint8_t blendedB = (uint8_t)min(255u,
                                         (uint16_t)max(eB, nB) + ((uint16_t)min(eB, nB) / 4u));

        strip.setPixelColor(idx, strip.Color(0, blendedG, blendedB));
      }
    }
  }
}

// ── Math ──────────────────────────────────────────────────────

float SimpleDecayMode::computeIntensity(const TouchDecayState& state, float t_ms) {
  //  I(t) = I₀ · e^(−t/τ)
  float baseDecay = state.I0_actual * expf(-t_ms / tau);
  float spring = massSpringResponse(t_ms, state.pressure);
  float springGain = 1.0f + 0.70f * spring;
  return baseDecay * springGain;
}
