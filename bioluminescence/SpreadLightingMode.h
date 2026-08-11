// ============================================================
// SpreadLightingMode.h
// Fourth mode: particle-based diffusion from the touched region.
// ============================================================
#ifndef SPREAD_LIGHTING_MODE_H
#define SPREAD_LIGHTING_MODE_H

#include "LightingMode.h"

struct SpreadParticle {
  float x;
  float y;
  float dx;
  float dy;
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint16_t life;
  uint16_t maxLife;
  float peakBrightness;
  bool active;
};

class SpreadLightingMode : public LightingMode {
public:
  SpreadLightingMode();

  void enter(Adafruit_NeoPixel& strip) override;
  void onTouch(Adafruit_NeoPixel& strip,
               const TouchEvent& event) override;
  void update(Adafruit_NeoPixel& strip) override;
  const char* getName() override { return "Spread Diffusion"; }

private:
  static const uint8_t kParticleCount = 40;

  SpreadParticle _particles[kParticleCount];
  uint32_t _noiseFrame;
  bool _active;

  void resetState();
  void trail(Adafruit_NeoPixel& strip);
  void spawnFrom(float x, float y, int16_t pressure);
  float fade(float t);
  float lerp(float a, float b, float t);
  float fract(float x);
  float hash2D(float x, float y);
  float perlinNoise2D(float x, float y);
  float calculateFireflyBrightness(uint16_t life, uint16_t maxLife, float peak);
};

#endif // SPREAD_LIGHTING_MODE_H