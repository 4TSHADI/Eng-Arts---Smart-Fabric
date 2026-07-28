// ============================================================
// ProjectConfig.h
// Shared hardware constants and math helpers for BioDecay.
//
// Hardware:  2× MPR121 on the same I²C bus (0x5A + 0x5B)
//            2× 16×16 NeoPixel panels chained → 16×32 grid
//
// Pin → 3×3 section mapping  (4-column × 3-row per MPR)
// ──────────────────────────────────────────────────────
// Each MPR121 owns one 16×16 panel.  Its 12 electrodes fill
// a 4-column × 3-row grid of 3×3 pixel sections:
//
//   electrode →  col = electrode % 4   row = electrode / 4
//   x0 = col * COL_STRIDE              (0, 4, 8, 12)
//   y0 = panelYBase + row * ROW_STRIDE (3-px rows within panel)
//
//   MPR1 (0x5A) global pins  0–11 → panel 0  (y  0–15)
//   MPR2 (0x5B) global pins 12–23 → panel 1  (y 16–31)
//
//   Pin layout within each panel:
//     pin  0  1  2  3   (row 0, y=0-2  / y=16-18)
//     pin  4  5  6  7   (row 1, y=5-7  / y=21-23)
//     pin  8  9 10 11   (row 2, y=10-12/ y=26-28)
//
// ── Panel chaining ───────────────────────────────────────────
// Set PANEL1_FLIP_Y = true if the second panel's data-in is at
// its bottom edge (common when panels are stacked with a short
// wire and the second panel's pixel 0 is at its bottom-left).
// ============================================================
#ifndef PROJECT_CONFIG_H
#define PROJECT_CONFIG_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>

// ── Hardware ─────────────────────────────────────────────────
#define LED_PIN              6
#define TCA9548A_ADDR        0x70
#define MPR121_ADDR          0x5A
#define NUM_MUX_CHANNELS     6
#define ELECTRODES_PER_MPR   12
#define NUM_PINS             (NUM_MUX_CHANNELS * ELECTRODES_PER_MPR)  // 72

// ── LED matrix ───────────────────────────────────────────────
// Two 16×16 NeoPixel panels chained → addressed as a single 16×32 grid.
const uint8_t  WIDTH     = 16;
const uint8_t  HEIGHT    = 32;
const uint16_t NUM_LEDS  = WIDTH * HEIGHT;  // 512

// ── Serpentine layout ────────────────────────────────────────
inline uint16_t XY(uint8_t x, uint8_t y) {
  if (x >= WIDTH || y >= HEIGHT) return NUM_LEDS;
  return (y & 1) ? (y * WIDTH + (WIDTH - 1 - x))
                : (y * WIDTH + x);
}

// ── Section geometry constants ────────────────────────────────
#define SECTION_SIZE    3   // 3×3 pixel block
#define COLS_PER_SLOT   4   // 4 columns per MPR
#define ROWS_PER_SLOT   3   // 3 rows per MPR
#define COL_STRIDE      4   // x-distance between section origins (3px + 1px gap)
#define ROW_STRIDE      2   // y-distance between row origins (allows 3 MPRs to fit on a 16px height)

// ── Per-MPR channel configuration ────────────────────────────
struct MprConfig {
  uint8_t matrixIndex;
  uint8_t slotIndex;
};

static constexpr MprConfig mprConfig[NUM_MUX_CHANNELS] = {
  { 0, 0 },
  { 0, 1 },
  { 0, 2 },
  { 1, 0 },
  { 1, 1 },
  { 1, 2 },
};

// ── Section lookup structs ────────────────────────────────────
struct PinSection {
  uint8_t xStart;
  uint8_t yStart;
  uint8_t xEnd;   // = xStart + SECTION_SIZE - 1  (always xStart + 2)
  uint8_t yEnd;   // = yStart + SECTION_SIZE - 1  (always yStart + 2)
};

struct PinRegion {
  float cx;       // section centre x
  float cy;       // section centre y
  float sigmaX;   // horizontal Gaussian spread
  float sigmaY;   // vertical   Gaussian spread
};

// ── getPinSection ─────────────────────────────────────────────
// Maps a global pin index (channel * ELECTRODES_PER_MPR + electrode)
// to its 3×3 pixel block on the correct LED matrix.
inline PinSection getPinSection(uint8_t globalPin) {
  uint8_t channel   = globalPin / ELECTRODES_PER_MPR;
  uint8_t electrode = globalPin % ELECTRODES_PER_MPR;

  uint8_t col = electrode % COLS_PER_SLOT;
  uint8_t row = electrode / COLS_PER_SLOT;

  uint8_t x0 = col * COL_STRIDE;

  uint8_t matrixYBase = mprConfig[channel].matrixIndex * 16;
  uint8_t slotYOffset = mprConfig[channel].slotIndex * 5;
  uint8_t y0 = matrixYBase + slotYOffset + (row * ROW_STRIDE);

  PinSection s;
  s.xStart = x0;
  s.yStart = y0;
  s.xEnd   = (x0 + SECTION_SIZE - 1 < WIDTH) ? x0 + SECTION_SIZE - 1 : WIDTH - 1;
  s.yEnd   = (y0 + SECTION_SIZE - 1 < HEIGHT) ? y0 + SECTION_SIZE - 1 : HEIGHT - 1;
  return s;
}

// ── getPinRegion ──────────────────────────────────────────────
// Derives Gaussian centre + spread from a pin's 3×3 section.
inline PinRegion getPinRegion(uint8_t globalPin) {
  PinSection s = getPinSection(globalPin);
  float w = (float)(s.xEnd - s.xStart + 1);  // always 3.0
  float h = (float)(s.yEnd - s.yStart + 1);  // always 3.0
  PinRegion r;
  r.cx     = s.xStart + (w - 1.0f) * 0.5f;
  r.cy     = s.yStart + (h - 1.0f) * 0.5f;
  r.sigmaX = max(1.0f, w * 0.4f);
  r.sigmaY = max(1.0f, h * 0.4f);
  return r;
}

// ── Gaussian radial brightness ───────────────────────────────
inline float gaussianBrightness(float dx, float dy,
                                 float sigX, float sigY) {
  float ex = (dx * dx) / (2.0f * sigX * sigX);
  float ey = (dy * dy) / (2.0f * sigY * sigY);
  return expf(-(ex + ey));
}

// ── Bioluminescent colour palette ────────────────────────────
// Deep-ocean cyan-blue: dinoflagellate / jellyfish hues.
inline uint32_t bioColor(Adafruit_NeoPixel& strip, uint8_t brightness) {
  uint8_t g = (uint8_t)((220UL * brightness) / 255);
  uint8_t b = (uint8_t)((255UL * brightness) / 255);
  return strip.Color(0, g, b);
}

#endif // PROJECT_CONFIG_H
