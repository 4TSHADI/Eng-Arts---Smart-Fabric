// ============================================================
// ProjectConfig.h
// Shared hardware constants and math helpers for BioDecay.
// Supports TWO MPR121 sensors (0x5A + 0x5B) on one I2C bus.
// ============================================================
#ifndef PROJECT_CONFIG_H
#define PROJECT_CONFIG_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>

// ── Hardware ─────────────────────────────────────────────────
#define LED_PIN        6
#define TCA9548A_ADDR  0x70
#define NUM_MUX_CHANNELS 6
#define MPR121_ADDR    0x5A
#define NUM_PINS       72    // 12 electrodes × 6 mux channels

// ── LED matrix ────────────────────────────────────────────────
// Start with a 16×32 canvas that can later be expanded for a larger tapestry.
const uint8_t  WIDTH    = 16;
const uint8_t  HEIGHT   = 32;
const uint16_t NUM_LEDS = WIDTH * HEIGHT; // 512

// ── Serpentine layout ────────────────────────────────────────
// Works for both panels since HEIGHT is now 32.
inline uint16_t XY(uint8_t x, uint8_t y) {
  if (x >= WIDTH || y >= HEIGHT) return NUM_LEDS; // guard: off-grid
  return (y & 1) ? (y * WIDTH + (WIDTH - 1 - x))
                 : (y * WIDTH + x);
}

// ── Pin → Region map ─────────────────────────────────────────
// 72 global pins, each mapped to a zone centre on the 16×32 canvas.
// The initial configuration uses six MPR121 sensors over a mux, laid out
// as a 3×2 grid of sensor clusters across the tapestry canvas.
//
// Each sensor contributes 12 electrodes, arranged in a 4×3 tile pattern.
// The region centers are distributed across the 16×32 canvas so the effect
// can scale to multiple matrices without needing a hard-coded per-sensor layout.
//
struct PinRegion {
  float cx;     // centre x  (float for Gaussian sub-pixel accuracy)
  float cy;     // centre y
  float sigmaX; // horizontal std-dev [pixels]
  float sigmaY; // vertical   std-dev [pixels]
};

const PinRegion PIN_REGIONS[NUM_PINS] = {
  // ── Panel 1  (MPR121 #1, pins 0-11) ──────────────────────
  //   cx      cy    sX    sY
  {  1.5f,  2.0f, 1.2f, 1.4f },  // pin  0
  {  5.5f,  2.0f, 1.2f, 1.4f },  // pin  1
  {  9.5f,  2.0f, 1.2f, 1.4f },  // pin  2
  { 13.5f,  2.0f, 1.2f, 1.4f },  // pin  3

  {  1.5f,  7.0f, 1.2f, 1.4f },  // pin  4
  {  5.5f,  7.0f, 1.2f, 1.4f },  // pin  5
  {  9.5f,  7.0f, 1.2f, 1.4f },  // pin  6
  { 13.5f,  7.0f, 1.2f, 1.4f },  // pin  7

  {  1.5f, 12.5f, 1.2f, 1.4f },  // pin  8
  {  5.5f, 12.5f, 1.2f, 1.4f },  // pin  9
  {  9.5f, 12.5f, 1.2f, 1.4f },  // pin 10
  { 13.5f, 12.5f, 1.2f, 1.4f },  // pin 11

  // ── Panel 2  (MPR121 #2, pins 12-23) ─────────────────────
  {  1.5f, 18.0f, 1.2f, 1.4f },  // pin 12
  {  5.5f, 18.0f, 1.2f, 1.4f },  // pin 13
  {  9.5f, 18.0f, 1.2f, 1.4f },  // pin 14
  { 13.5f, 18.0f, 1.2f, 1.4f },  // pin 15

  {  1.5f, 23.0f, 1.2f, 1.4f },  // pin 16
  {  5.5f, 23.0f, 1.2f, 1.4f },  // pin 17
  {  9.5f, 23.0f, 1.2f, 1.4f },  // pin 18
  { 13.5f, 23.0f, 1.2f, 1.4f },  // pin 19

  {  1.5f, 28.5f, 1.2f, 1.4f },  // pin 20
  {  5.5f, 28.5f, 1.2f, 1.4f },  // pin 21
  {  9.5f, 28.5f, 1.2f, 1.4f },  // pin 22
  { 13.5f, 28.5f, 1.2f, 1.4f },  // pin 23
};

// ── Gaussian radial brightness ───────────────────────────────
// Returns [0.0, 1.0] — multiply by peak intensity to get pixel brightness.
inline float gaussianBrightness(float dx, float dy,
                                 float sigX, float sigY) {
  float ex = (dx * dx) / (2.0f * sigX * sigX);
  float ey = (dy * dy) / (2.0f * sigY * sigY);
  return expf(-(ex + ey));
}

// ── Bioluminescent colour palette ────────────────────────────
// Deep-ocean cyan-blue, matching real dinoflagellate / jellyfish hues.
inline uint32_t bioColor(Adafruit_NeoPixel& strip, uint8_t brightness) {
  uint8_t g = (uint8_t)((220UL * brightness) / 255);
  uint8_t b = (uint8_t)((255UL * brightness) / 255);
  return strip.Color(0, g, b);
}

#endif // PROJECT_CONFIG_H
