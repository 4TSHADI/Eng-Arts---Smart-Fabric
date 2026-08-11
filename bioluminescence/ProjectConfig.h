// ============================================================
// ProjectConfig.h
// Shared hardware constants and math helpers for BioLighting.
//
// Hardware:
//   - 4× MPR121 behind a TCA9548A mux
//   - Active touch input uses mux channels 2 (X) and 3 (Y)
//   - One sensor pair controls both LED panels
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
#define NUM_MUX_CHANNELS     4
#define ELECTRODES_PER_MPR   12

// Touch sensitivity thresholds (lower values are more sensitive)
// Default: Touch = 12, Release = 6. Previous: Touch = 8, Release = 4.
#define MPR121_TOUCH_THRESHOLD    4
#define MPR121_RELEASE_THRESHOLD  2
#define SENSOR_STARTUP_CALIBRATION_MS 30000

// Runtime feature toggles
#define ENABLE_WIFI_CONTROL        0
#define SENSOR_STARTUP_DEEP_SCAN   0
#define TOUCH_EVENT_SERIAL_LOG     1
#define MODE_EVENT_SERIAL_LOG      0
#define SERIAL_WAIT_TIMEOUT_MS     2000


#define TOUCH_GRID_SIZE      8
#define ELECTRODES_PER_SENSOR TOUCH_GRID_SIZE
#define NUM_TOUCH_CELLS      (TOUCH_GRID_SIZE * TOUCH_GRID_SIZE)
#define NUM_TOUCH_POINTS     (NUM_PANELS * NUM_TOUCH_CELLS)
#define TOUCH_REGION_SIZE    2
#define DISABLED_ELECTRODE   255

static constexpr uint8_t X_SENSOR_CHANNELS[NUM_PANELS] = {2, 2};
static constexpr uint8_t Y_SENSOR_CHANNELS[NUM_PANELS] = {3, 3};

// Per-sensor electrode-to-cell mapping (up to 8 active electrodes per MPR121).
// Cell index is the position in each row below (0..7).
// Use DISABLED_ELECTRODE (255) to switch an electrode off completely.
static constexpr uint8_t SENSOR_ELECTRODE_MAP[NUM_MUX_CHANNELS][ELECTRODES_PER_SENSOR] = {
  // Unused channels (0 and 1)
  {DISABLED_ELECTRODE, DISABLED_ELECTRODE, DISABLED_ELECTRODE, DISABLED_ELECTRODE, DISABLED_ELECTRODE, DISABLED_ELECTRODE, DISABLED_ELECTRODE, DISABLED_ELECTRODE},
  {DISABLED_ELECTRODE, DISABLED_ELECTRODE, DISABLED_ELECTRODE, DISABLED_ELECTRODE, DISABLED_ELECTRODE, DISABLED_ELECTRODE, DISABLED_ELECTRODE, DISABLED_ELECTRODE},
  // Active X sensor on mux2: mapped from electrode 4
  {8,9,10,11,0,1,2,3},
  // Active Y sensor on mux3
  {5,4, 3, 2, 1, 0, 11, 10},
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
const uint8_t  WIDTH     = 32;
const uint8_t  HEIGHT    = 16;
const uint16_t NUM_LEDS  = WIDTH * HEIGHT;

// ── Serpentine layout ────────────────────────────────────────
inline uint16_t XY(uint8_t x, uint8_t y) {
  if (x >= WIDTH || y >= HEIGHT) return NUM_LEDS;
  if (x < 16) {
    // Panel 0 (left) - first 256 LEDs
    return (y & 1) ? (y * 16 + (15 - x))
                   : (y * 16 + x);
  } else {
    // Panel 1 (right) - second 256 LEDs
    uint8_t localX = x - 16;
    return 256 + ((y & 1) ? (y * 16 + (15 - localX))
                          : (y * 16 + localX));
  }
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

  uint8_t xBase = (uint8_t)(panelId * 16);
  uint8_t x0 = min((uint8_t)(xBase + xCell * TOUCH_REGION_SIZE), (uint8_t)(WIDTH - TOUCH_REGION_SIZE));
  uint8_t y0 = min((uint8_t)(yCell * TOUCH_REGION_SIZE), (uint8_t)(HEIGHT - TOUCH_REGION_SIZE));

  PinSection s;
  s.xStart = x0;
  s.yStart = y0;
  s.xEnd = min((uint8_t)(x0 + TOUCH_REGION_SIZE - 1), (uint8_t)(WIDTH - 1));
  s.yEnd = min((uint8_t)(y0 + TOUCH_REGION_SIZE - 1), (uint8_t)(HEIGHT - 1));
  return s;
}

inline PinRegion getTouchRegion(uint8_t panelId, uint8_t xCell, uint8_t yCell) {
  PinSection s = getTouchRegionBounds(panelId, xCell, yCell);
  float w = (float)(s.xEnd - s.xStart + 1);  // usually 2.0 with TOUCH_REGION_SIZE=2
  float h = (float)(s.yEnd - s.yStart + 1);  // usually 2.0 with TOUCH_REGION_SIZE=2
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
