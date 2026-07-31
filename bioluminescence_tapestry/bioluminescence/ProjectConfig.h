// ============================================================
// ProjectConfig.h
// Shared hardware constants and math helpers for BioDecay.
//
// Coordinate system used here:
// - X axis uses 6 pins: 0..5 (right to left)
// - Y axis uses 6 pins: 6..11 (top to bottom)
// - Each LED panel uses one MPR121 with this split.
// - Two 16x16 LED panels are stacked and addressed as 16x32.
// - Touches are handled as XY cells, and each cell maps to a compact
//   2x2 LED region that starts at coordinates like (0,0), (0,2), (0,4).
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
#define NUM_PANELS           2
#define NUM_MUX_CHANNELS     NUM_PANELS
#define ELECTRODES_PER_MPR   12

// Each panel has a 6x6 XY intersection grid => 36 touch cells.
#define CELLS_PER_AXIS       6
#define TOUCH_POINTS_PER_PANEL (CELLS_PER_AXIS * CELLS_PER_AXIS)
#define NUM_TOUCH_POINTS     (NUM_PANELS * TOUCH_POINTS_PER_PANEL)
#define TOUCH_REGION_SIZE    2
#define TOUCH_COORD_STEP     2

// ── LED matrix ────────────────────────────────────────────────
// Two stacked 16x16 panels -> 16x32 canvas.
const uint8_t  WIDTH    = 16;
const uint8_t  HEIGHT   = 32;
const uint16_t NUM_LEDS = WIDTH * HEIGHT; // 512

// ── Serpentine layout ────────────────────────────────────────
inline uint16_t XY(uint8_t x, uint8_t y) {
  if (x >= WIDTH || y >= HEIGHT) return NUM_LEDS; // guard: off-grid
  return (y & 1) ? (y * WIDTH + (WIDTH - 1 - x))
                 : (y * WIDTH + x);
}

// ── Shared touch-region helpers ──────────────────────────────
struct PinSection {
  uint8_t xStart;
  uint8_t yStart;
  uint8_t xEnd;
  uint8_t yEnd;
};

struct PinRegion {
  float cx;
  float cy;
  float sigmaX;
  float sigmaY;
};

inline uint8_t axisCellToPixel(uint8_t axisCell) {
  // 6 cells -> LED coordinates 0,2,4,6,8,10.
  if (axisCell > 5) axisCell = 5;
  return (uint8_t)(axisCell * TOUCH_COORD_STEP);
}

inline uint16_t touchPointIndex(uint8_t panelId, uint8_t xCell, uint8_t yCell) {
  if (panelId >= NUM_PANELS) panelId = NUM_PANELS - 1;
  if (xCell >= CELLS_PER_AXIS) xCell = CELLS_PER_AXIS - 1;
  if (yCell >= CELLS_PER_AXIS) yCell = CELLS_PER_AXIS - 1;
  return (uint16_t)(panelId * TOUCH_POINTS_PER_PANEL + yCell * CELLS_PER_AXIS + xCell);
}

inline uint8_t touchPointX(uint16_t touchPoint) {
  return (uint8_t)(touchPoint % CELLS_PER_AXIS);
}

inline uint8_t touchPointY(uint16_t touchPoint) {
  return (uint8_t)((touchPoint / CELLS_PER_AXIS) % CELLS_PER_AXIS);
}

inline uint8_t touchPointPanel(uint16_t touchPoint) {
  return (uint8_t)(touchPoint / TOUCH_POINTS_PER_PANEL);
}

inline void getTouchOrigin(uint8_t panelId, uint8_t xCell, uint8_t yCell,
                           uint8_t& x, uint8_t& y) {
  if (panelId >= NUM_PANELS) panelId = NUM_PANELS - 1;
  x = axisCellToPixel(xCell);
  y = (uint8_t)(panelId * 16 + axisCellToPixel(yCell));
}

inline PinSection getTouchRegionBounds(uint8_t panelId, uint8_t xCell, uint8_t yCell) {
  uint8_t x0, y0;
  getTouchOrigin(panelId, xCell, yCell, x0, y0);

  PinSection section;
  section.xStart = x0;
  section.yStart = y0;
  section.xEnd = min((uint8_t)(x0 + TOUCH_REGION_SIZE - 1), (uint8_t)(WIDTH - 1));
  section.yEnd = min((uint8_t)(y0 + TOUCH_REGION_SIZE - 1), (uint8_t)(HEIGHT - 1));
  return section;
}

inline PinSection getPinSection(uint8_t pin) {
  uint8_t panel = pin / TOUCH_POINTS_PER_PANEL;
  uint8_t local = pin % TOUCH_POINTS_PER_PANEL;
  uint8_t xCell = local % CELLS_PER_AXIS;
  uint8_t yCell = local / CELLS_PER_AXIS;

  return getTouchRegionBounds(panel, xCell, yCell);
}

inline PinRegion getTouchRegion(uint8_t panelId, uint8_t xCell, uint8_t yCell) {
  PinSection s = getTouchRegionBounds(panelId, xCell, yCell);
  float width = (float)(s.xEnd - s.xStart + 1);
  float height = (float)(s.yEnd - s.yStart + 1);

  PinRegion r;
  r.cx = s.xStart + (width - 1.0f) * 0.5f;
  r.cy = s.yStart + (height - 1.0f) * 0.5f;
  r.sigmaX = max(1.0f, width * 0.4f);
  r.sigmaY = max(1.0f, height * 0.4f);
  return r;
}

inline PinRegion getPinRegion(uint8_t pin) {
  uint8_t panel = pin / TOUCH_POINTS_PER_PANEL;
  uint8_t local = pin % TOUCH_POINTS_PER_PANEL;
  return getTouchRegion(panel, local % CELLS_PER_AXIS, local / CELLS_PER_AXIS);
}

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
