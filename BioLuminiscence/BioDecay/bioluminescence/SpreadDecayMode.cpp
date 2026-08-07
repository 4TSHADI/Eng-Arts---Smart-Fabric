#include "SpreadDecayMode.h"

SpreadDecayMode::SpreadDecayMode()
  : _noiseFrame(0), _active(false) {
  resetState();
}

void SpreadDecayMode::resetState() {
  for (uint8_t i = 0; i < kParticleCount; i++) {
    _particles[i].active = false;
    _particles[i].life = 0;
    _particles[i].maxLife = 0;
    _particles[i].peakBrightness = 0.0f;
  }
}

void SpreadDecayMode::enter(Adafruit_NeoPixel& strip) {
  strip.setBrightness(50);
  strip.clear();
  strip.show();
  resetState();
  _active = false;
  _noiseFrame = 0;

  Serial.println("[SpreadDecay] Entered – Firefly mode.");
}

void SpreadDecayMode::trail(Adafruit_NeoPixel& strip) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    uint32_t c = strip.getPixelColor(i);
    uint8_t r = (c >> 16) & 0xFF;
    uint8_t g = (c >> 8) & 0xFF;
    uint8_t b = c & 0xFF;

    r = (uint8_t)((r * 210) / 255);
    g = (uint8_t)((g * 210) / 255);
    b = (uint8_t)((b * 210) / 255);

    strip.setPixelColor(i, strip.Color(r, g, b));
  }
}

void SpreadDecayMode::spawnFrom(float x, float y, int16_t pressure) {
  const uint8_t fireflyR = 255;
  const uint8_t fireflyG = 70;
  const uint8_t fireflyB = 235;
  float springKick = massSpringResponse(120.0f, pressure);
  float speedGain = 1.0f + 0.45f * springKick;
  float lifeGain = 1.0f + 0.35f * springKick;

  for (uint8_t i = 0; i < kParticleCount; i++) {
    float angle = random(0, 360) * PI / 180.0f;
    float speed = (random(15, 60) / 100.0f) * speedGain;

    _particles[i].x = x;
    _particles[i].y = y;
    _particles[i].dx = cosf(angle) * speed;
    _particles[i].dy = sinf(angle) * speed;
    _particles[i].r = fireflyR;
    _particles[i].g = fireflyG;
    _particles[i].b = fireflyB;
    _particles[i].life = 0;
    _particles[i].maxLife = (uint16_t)(random(30, 70) * lifeGain);
    _particles[i].peakBrightness = constrain(0.35f + (pressure / 255.0f) * 0.45f + 0.45f * springKick,
                                             0.0f,
                                             1.25f);
    _particles[i].active = true;
  }

  _active = true;
}

void SpreadDecayMode::onTouch(Adafruit_NeoPixel& strip,
                              const TouchEvent& event) {
  if (!event.isTouched) return;
  if (event.xCell >= TOUCH_GRID_SIZE || event.yCell >= TOUCH_GRID_SIZE) return;

  PinSection section = getTouchRegionBounds(event.panelId, event.xCell, event.yCell);
  float spawnX = (float)section.xStart;
  float spawnY = (float)section.yStart;
  spawnFrom(spawnX, spawnY, event.pressure);
}

float SpreadDecayMode::fade(float t) {
  return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float SpreadDecayMode::lerp(float a, float b, float t) {
  return a + t * (b - a);
}

float SpreadDecayMode::fract(float x) {
  return x - floor(x);
}

float SpreadDecayMode::hash2D(float x, float y) {
  return fract(sinf(x * 12.9898f + y * 78.233f) * 43758.5453f);
}

float SpreadDecayMode::perlinNoise2D(float x, float y) {
  int x0 = floor(x);
  int y0 = floor(y);
  float xf = x - x0;
  float yf = y - y0;

  float u = fade(xf);
  float v = fade(yf);

  float n00 = hash2D(x0, y0);
  float n10 = hash2D(x0 + 1, y0);
  float n01 = hash2D(x0, y0 + 1);
  float n11 = hash2D(x0 + 1, y0 + 1);

  float x1 = lerp(n00, n10, u);
  float x2 = lerp(n01, n11, u);
  return lerp(x1, x2, v) * 2.0f - 1.0f;
}

float SpreadDecayMode::calculateFireflyBrightness(uint16_t life, uint16_t maxLife, float peak) {
  if (maxLife == 0) return 0.0f;

  float t = (float)life / (float)maxLife;
  float tPeak = 0.25f;
  float intensity = 0.0f;

  if (t <= tPeak) {
    float normRise = t / tPeak;
    intensity = normRise * normRise * (3.0f - 2.0f * normRise);
  } else {
    float normDecay = (t - tPeak) / (1.0f - tPeak);
    float remaining = 1.0f - normDecay;
    intensity = remaining * remaining * remaining;
  }

  return intensity * peak;
}

void SpreadDecayMode::update(Adafruit_NeoPixel& strip) {
  if (!_active) return;

  trail(strip);
  bool anyActive = false;

  for (uint8_t i = 0; i < kParticleCount; i++) {
    if (!_particles[i].active) continue;

    anyActive = true;
    _particles[i].life++;

    float angleNoise = perlinNoise2D(_particles[i].x * 0.12f + _noiseFrame * 0.015f,
                                     _particles[i].y * 0.12f + _noiseFrame * 0.015f) * PI * 2.0f;

    _particles[i].dx += cosf(angleNoise) * 0.03f;
    _particles[i].dy += sinf(angleNoise) * 0.03f;
    _particles[i].dx *= 0.96f;
    _particles[i].dy *= 0.96f;

    _particles[i].x += _particles[i].dx;
    _particles[i].y += _particles[i].dy;

    int xx = (int)roundf(_particles[i].x);
    int yy = (int)roundf(_particles[i].y);

    if (xx < 0 || xx >= WIDTH || yy < 0 || yy >= HEIGHT || _particles[i].life >= _particles[i].maxLife) {
      _particles[i].active = false;
      continue;
    }

    float brightnessFactor = calculateFireflyBrightness(_particles[i].life,
                                                        _particles[i].maxLife,
                                                        _particles[i].peakBrightness);

    if (brightnessFactor > 0.001f) {
      uint8_t r = (uint8_t)((_particles[i].r * brightnessFactor * 0.95f));
      uint8_t g = (uint8_t)((_particles[i].g * brightnessFactor * 0.40f));
      uint8_t b = (uint8_t)((_particles[i].b * brightnessFactor * 1.10f));

      strip.setPixelColor(XY((uint8_t)xx, (uint8_t)yy), strip.Color(r, g, b));
    } else {
      _particles[i].active = false;
    }
  }

  strip.show();
  _noiseFrame++;

  if (!anyActive) {
    _active = false;
    strip.clear();
    strip.show();
  }
}
