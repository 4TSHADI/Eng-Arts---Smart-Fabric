// ============================================================
// SimpleDecayMode.cpp
// MODEL 1 – I(t) = I₀ · e^(−t/τ)
//
// Per-pin behaviour:
//   • Touch  → start an independent exponential decay for that pin
//   • update() → for every active pin, evaluate I(t) and paint its
//                region onto the frame buffer with a Gaussian
//                brightness gradient (bright centre, dark edges)
//   • Multiple pins can glow simultaneously
// ============================================================
#include "SimpleDecayMode.h"

// ── Lifecycle ─────────────────────────────────────────────────

void SimpleDecayMode::enter(Adafruit_NeoPixel& strip) {
  strip.clear();
  strip.show();

  // Reset all per-pin state
  for (uint8_t p = 0; p < NUM_PINS; p++) {
    _pins[p].active    = false;
    _pins[p].startTime = 0;
    _pins[p].I0_actual = 0.0f;
  }

  Serial.println("[SimpleDecay] Entered – 4x3 Gaussian regions active.");
  Serial.println("             Touch any electrode to trigger its zone.");
}

void SimpleDecayMode::onTouch(Adafruit_NeoPixel& strip,
                               uint8_t pin, bool isTouched, int16_t pressure) {
  if (pin >= NUM_PINS) return;

  if (isTouched) {
    // Scale peak brightness with pressure; floor at 80 so a light touch
    // is still clearly visible.
    _pins[pin].I0_actual = constrain(map(pressure, 0, 150, 80, 255), 80, 255);
    _pins[pin].startTime = millis();
    _pins[pin].active    = true;

    Serial.print("[SimpleDecay] pin="); Serial.print(pin);
    Serial.print("  pressure=");        Serial.print(pressure);
    Serial.print("  I0=");              Serial.println(_pins[pin].I0_actual);
  }
  // Finger-up: let the decay finish naturally (don't kill it early)
}

void SimpleDecayMode::update(Adafruit_NeoPixel& strip) {
  // Check if anything is active
  bool anyActive = false;
  for (uint8_t p = 0; p < NUM_PINS; p++) {
    if (_pins[p].active) { anyActive = true; break; }
  }
  if (!anyActive) return;

  // Rebuild the frame from scratch each tick so decayed pins fade cleanly
  strip.clear();
  renderFrame(strip);
  strip.show();
}

// ── Rendering ────────────────────────────────────────────────

void SimpleDecayMode::renderFrame(Adafruit_NeoPixel& strip) {
  unsigned long now = millis();

  for (uint8_t p = 0; p < NUM_PINS; p++) {
    if (!_pins[p].active) continue;

    float t_ms = (float)(now - _pins[p].startTime);
    float I    = computeIntensity(_pins[p], t_ms); // peak intensity 0-255

    if (I < 1.0f) {
      // Decay finished – silence this pin
      _pins[p].active = false;
      Serial.print("[SimpleDecay] pin="); Serial.print(p);
      Serial.println(" decay complete.");
      continue;
    }

    // ── Gaussian region draw ───────────────────────────────
    const PinRegion& region = PIN_REGIONS[p];

    // Broaden the glow slightly so the colour diffuses outward more cleanly.
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
        gau = powf(gau, 0.85f); // soften the falloff for a cleaner edge

        // Pixel brightness = decayed intensity × softened Gaussian falloff
        float scaledI = I * gau;
        if (scaledI < 1.0f) continue; // skip fully dark pixels

        uint8_t bri = (uint8_t)constrain(scaledI, 0.0f, 255.0f);

        uint16_t idx = XY((uint8_t)x, (uint8_t)y);
        if (idx >= NUM_LEDS) continue;

        // Accumulate softly so overlapping glows blend instead of snapping.
        uint32_t existing = strip.getPixelColor(idx);
        uint32_t newColor  = bioColor(strip, bri);

        // Extract green & blue channels (no red in our palette)
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

float SimpleDecayMode::computeIntensity(const PinDecay& pd, float t_ms) {
  //  I(t) = I₀ · e^(−t/τ)
  return pd.I0_actual * expf(-t_ms / tau);
}
