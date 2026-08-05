// ============================================================
// ProjectConfig.h
// Shared hardware constants and math helpers for BioDecay.
//
// Hardware:
//   - MPR121 sensors behind a 6-channel TCA9548A mux
//   - 2× 16×16 LED panels chained as a 16×32 canvas
//   - Each LED panel is driven by one X-axis MPR121 and one Y-axis MPR121
//   - Every panel therefore gets an independent 8×8 touch grid
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
#define NUM_MUX_CHANNELS     6
#define ELECTRODES_PER_MPR   12

// Four MPR121 sensors are arranged as two X/Y pairs:
// panel 0 -> channels 0 (X), 1 (Y)
// panel 1 -> channels 2 (X), 3 (Y)
#define TOUCH_GRID_SIZE      8
#define ELECTRODES_PER_SENSOR TOUCH_GRID_SIZE
#define NUM_TOUCH_CELLS      (TOUCH_GRID_SIZE * TOUCH_GRID_SIZE)
#define NUM_TOUCH_POINTS     (NUM_PANELS * NUM_TOUCH_CELLS)
#define TOUCH_REGION_SIZE    2

static constexpr uint8_t X_SENSOR_CHANNELS[NUM_PANELS] = {0, 2};
static constexpr uint8_t Y_SENSOR_CHANNELS[NUM_PANELS] = {1, 3};

// Per-sensor electrode-to-cell mapping (8 active electrodes per MPR121).
// Cell index is the position in each row below (0..7).
//
// Requested layout:
//   Sensor 0: 0,1,2,3,4,9,10,11
//   Sensor 1: 0,1,2,3,8,9,10,11
//
// Channels 2 and 3 mirror channels 0 and 1.
// Channels 4 and 5 are spare on the 6-channel mux.
static constexpr uint8_t SENSOR_ELECTRODE_MAP[NUM_MUX_CHANNELS][ELECTRODES_PER_SENSOR] = {
  {3,2,1,0,11,10,9,8},
  {9,10,11,0,1,2,3,4},
  {0, 1, 2, 3, 4, 9, 10, 11},
  {0, 1, 2, 3, 8, 9, 10, 11},
  {0, 1, 2, 3, 4, 9, 10, 11},
  {0, 1, 2, 3, 8, 9, 10, 11}
};

// ── WiFi configuration ──────────────────────────────────────
#ifndef WIFI_SSID
#define WIFI_SSID "Amantle"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "MadiAKae"
#endif

#define WIFI_HTTP_PORT 80

// ── LED matrix ───────────────────────────────────────────────
// Two 16×16 NeoPixel panels chained → addressed as a single 16×32 grid.
const uint8_t  WIDTH     = 16;
const uint8_t  HEIGHT    = 32;
const uint16_t NUM_LEDS  = WIDTH * HEIGHT;

// ── Serpentine layout ────────────────────────────────────────
inline uint16_t XY(uint8_t x, uint8_t y) {
  if (x >= WIDTH || y >= HEIGHT) return NUM_LEDS;
  return (y & 1) ? (y * WIDTH + (WIDTH - 1 - x))
                : (y * WIDTH + x);
}

// ── Section lookup structs ────────────────────────────────────
struct PinSection {
  uint8_t xStart;
  uint8_t yStart;
  uint8_t xEnd;
  uint8_t yEnd;
};

struct PinRegion {
  float cx;       // section centre x
  float cy;       // section centre y
  float sigmaX;   // horizontal Gaussian spread
  float sigmaY;   // vertical   Gaussian spread
};

inline uint16_t touchPointIndex(uint8_t panelId, uint8_t xCell, uint8_t yCell) {
  if (panelId >= NUM_PANELS) panelId = NUM_PANELS - 1;
  if (xCell >= TOUCH_GRID_SIZE) xCell = TOUCH_GRID_SIZE - 1;
  if (yCell >= TOUCH_GRID_SIZE) yCell = TOUCH_GRID_SIZE - 1;
  return (uint16_t)(panelId * NUM_TOUCH_CELLS + yCell * TOUCH_GRID_SIZE + xCell);
}

inline uint8_t touchPointPanel(uint16_t touchPoint) {
  return (uint8_t)(touchPoint / NUM_TOUCH_CELLS);
}

inline uint8_t touchPointX(uint16_t touchPoint) {
  return (uint8_t)(touchPoint % TOUCH_GRID_SIZE);
}

inline uint8_t touchPointY(uint16_t touchPoint) {
  return (uint8_t)((touchPoint / TOUCH_GRID_SIZE) % TOUCH_GRID_SIZE);
}

inline PinSection getTouchRegionBounds(uint8_t panelId, uint8_t xCell, uint8_t yCell) {
  if (panelId >= NUM_PANELS) panelId = NUM_PANELS - 1;
  if (xCell >= TOUCH_GRID_SIZE) xCell = TOUCH_GRID_SIZE - 1;
  if (yCell >= TOUCH_GRID_SIZE) yCell = TOUCH_GRID_SIZE - 1;

  uint8_t x0 = min((uint8_t)(xCell * TOUCH_REGION_SIZE), (uint8_t)(WIDTH - TOUCH_REGION_SIZE));
  uint8_t yBase = (uint8_t)(panelId * 16);
  uint8_t y0 = min((uint8_t)(yBase + yCell * TOUCH_REGION_SIZE), (uint8_t)(HEIGHT - TOUCH_REGION_SIZE));

  PinSection s;
  s.xStart = x0;
  s.yStart = y0;
  s.xEnd = min((uint8_t)(x0 + TOUCH_REGION_SIZE - 1), (uint8_t)(WIDTH - 1));
  s.yEnd = min((uint8_t)(y0 + TOUCH_REGION_SIZE - 1), (uint8_t)(HEIGHT - 1));
  return s;
}

inline PinRegion getTouchRegion(uint8_t panelId, uint8_t xCell, uint8_t yCell) {
  PinSection s = getTouchRegionBounds(panelId, xCell, yCell);
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
