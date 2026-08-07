// ============================================================
// MultiExpDecayMode.cpp
// MODEL 3 – SimpleDecay + dual-time-constant tail
// I(t) = scale * [A1*e^(−t/tau1) + A2*e^(−t/tau2)]
// ============================================================
#include "MultiExpDecayMode.h"

namespace {
float hash01(uint32_t n) {
  float x = sinf((float)n * 12.9898f) * 43758.5453f;
  return x - floorf(x);
}
}

// ── Lifecycle ─────────────────────────────────────────────────

void MultiExpDecayMode::enter(Adafruit_NeoPixel& strip) {
  SimpleDecayMode::enter(strip);

  tau = tau1;
  _scale        = 1.0f;

  Serial.println("[MultiExpDecay] Entered. Inherits SimpleDecay, adds slow tail.");
  Serial.print("  Fast: A1="); Serial.print(A1); Serial.print("  tau1="); Serial.print(tau1); Serial.println("ms");
  Serial.print("  Slow: A2="); Serial.print(A2); Serial.print("  tau2="); Serial.print(tau2); Serial.println("ms");
}

void MultiExpDecayMode::onTouch(Adafruit_NeoPixel& strip,
                                 const TouchEvent& event) {
  if (event.isTouched) {
    _scale = constrain(map(event.pressure, 0, 255, 50, 100), 50, 100) / 100.0f;
  }
  SimpleDecayMode::onTouch(strip, event);
}

void MultiExpDecayMode::update(Adafruit_NeoPixel& strip) {
  bool anyActive = false;
  for (uint16_t p = 0; p < NUM_TOUCH_POINTS; p++) {
    if (_touchStates[p].active) { anyActive = true; break; }
  }
  if (!anyActive) return;

  strip.clear();

  unsigned long now = millis();
  for (uint16_t p = 0; p < NUM_TOUCH_POINTS; p++) {
    if (!_touchStates[p].active) continue;

    float t_ms = (float)(now - _touchStates[p].startTime);

    // Use the base model as a low-cost completion check.
    float baseI = computeIntensity(_touchStates[p], t_ms);
    if (baseI < 0.8f) {
      _touchStates[p].active = false;
      continue;
    }

    renderBlobField(strip, p, _touchStates[p], t_ms);
  }

  strip.show();
}

// ── Math ──────────────────────────────────────────────────────

float MultiExpDecayMode::computeIntensity(const TouchDecayState& state, float t_ms) {
  float fast = _scale * A1 * expf(-t_ms / tau1);
  float slow = _scale * A2 * expf(-t_ms / tau2);
  float spring = massSpringResponse(t_ms, state.pressure);
  float springGain = 1.0f + 0.60f * spring;
  return (fast + slow) * springGain;
}

void MultiExpDecayMode::renderBlobField(Adafruit_NeoPixel& strip,
                                        uint16_t touchPoint,
                                        const TouchDecayState& state,
                                        float t_ms) {
  PinRegion region = getTouchRegion(touchPointPanel(touchPoint),
                                    touchPointX(touchPoint),
                                    touchPointY(touchPoint));

  float pressureNorm = constrain((float)state.pressure / 255.0f, 0.0f, 1.0f);
  float spring = massSpringResponse(t_ms, state.pressure);
  float springGain = 1.0f + 0.60f * spring;

  for (uint8_t b = 0; b < kBlobCount; ++b) {
    uint32_t seed = (uint32_t)touchPoint * 131u + (uint32_t)b * 977u + 17u;
    float h0 = hash01(seed + 1u);
    float h1 = hash01(seed + 2u);
    float h2 = hash01(seed + 3u);
    float h3 = hash01(seed + 4u);
    float h4 = hash01(seed + 5u);

    float angle = h0 * 2.0f * PI;
    float radial = (0.30f + 0.95f * h1) * (1.5f + 4.1f * pressureNorm);
    float cx = region.cx + cosf(angle) * radial;
    float cy = region.cy + sinf(angle) * radial;

    float tauFast = tau1 * (0.45f + 1.35f * h2);
    float tauSlow = tau2 * (0.55f + 1.25f * h3);
    float blobWeight = 0.40f + 0.90f * h4;

    float fast = _scale * A1 * expf(-t_ms / tauFast);
    float slow = _scale * A2 * expf(-t_ms / tauSlow);
    float I = (fast + slow) * (1.08f * springGain) * blobWeight;

    if (I < 0.8f) continue;

    float spreadX = region.sigmaX * (0.65f + 1.30f * h2) * (0.90f + 0.95f * pressureNorm);
    float spreadY = region.sigmaY * (0.65f + 1.30f * h3) * (0.90f + 0.95f * pressureNorm);

    int xMin = max(0, (int)(cx - 4.0f * spreadX));
    int xMax = min((int)WIDTH  - 1, (int)(cx + 4.0f * spreadX));
    int yMin = max(0, (int)(cy - 4.0f * spreadY));
    int yMax = min((int)HEIGHT - 1, (int)(cy + 4.0f * spreadY));

    for (int y = yMin; y <= yMax; y++) {
      for (int x = xMin; x <= xMax; x++) {
        float dx = (float)x - cx;
        float dy = (float)y - cy;
        float gau = gaussianBrightness(dx, dy, spreadX, spreadY);
        gau = powf(gau, 0.86f);

        float scaledI = I * gau;
        if (scaledI < 1.0f) continue;

        uint8_t bri = (uint8_t)constrain(scaledI, 0.0f, 255.0f);
        uint16_t idx = XY((uint8_t)x, (uint8_t)y);
        if (idx >= NUM_LEDS) continue;

        uint32_t existing = strip.getPixelColor(idx);
        uint32_t newColor = bioColor(strip, bri);

        uint8_t eG = (existing >> 8) & 0xFF;
        uint8_t eB = existing & 0xFF;
        uint8_t nG = (newColor >> 8) & 0xFF;
        uint8_t nB = newColor & 0xFF;

        uint8_t blendedG = (uint8_t)min(255u,
                                        (uint16_t)max(eG, nG) + ((uint16_t)min(eG, nG) / 4u));
        uint8_t blendedB = (uint8_t)min(255u,
                                        (uint16_t)max(eB, nB) + ((uint16_t)min(eB, nB) / 4u));

        strip.setPixelColor(idx, strip.Color(0, blendedG, blendedB));
      }
    }
  }
}
