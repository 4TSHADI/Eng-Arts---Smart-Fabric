// ============================================================
// ProjectConfig.h
// Shared hardware constants and math helpers for BioDecay.
// Supports 3 panels with 2 MPR121 sensors per panel.
// Each MPR121 sits on its own TCA9548A channel (6 channels total).
// ============================================================
#ifndef PROJECT_CONFIG_H
#define PROJECT_CONFIG_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>

// ── Hardware ─────────────────────────────────────────────────
#define LED_PIN        6
#define TCA9548A_ADDR  0x70
#define NUM_PANELS      3
#define MPR_PER_PANEL   2
#define ELECTRODES_PER_MPR 12
#define NUM_MPR_SENSORS (NUM_PANELS * MPR_PER_PANEL)
#define NUM_MUX_CHANNELS NUM_MPR_SENSORS
#define NUM_PINS        (NUM_MPR_SENSORS * ELECTRODES_PER_MPR)

// UNO R4 WiFi mode-switch control
// Set your local WiFi credentials here.
const char WIFI_SSID[] = "Amantle";
const char WIFI_PASS[] = "MadiAKae";
const uint16_t WIFI_HTTP_PORT = 80;

#define MPR121_ADDR 0x5A

struct SensorBinding {
  uint8_t muxChannel;
  uint8_t panelId;
  uint8_t axisId;   // 0 = X, 1 = Y
  uint8_t i2cAddr;
};

const SensorBinding SENSOR_BINDINGS[NUM_MPR_SENSORS] = {
  { 0, 0, 0, MPR121_ADDR },
  { 1, 0, 1, MPR121_ADDR },
  { 2, 1, 0, MPR121_ADDR },
  { 3, 1, 1, MPR121_ADDR },
  { 4, 2, 0, MPR121_ADDR },
  { 5, 2, 1, MPR121_ADDR },
};

// ── LED matrix ────────────────────────────────────────────────
// Start with a 16×32 canvas that can later be expanded for a larger tapestry.
const uint8_t  WIDTH    = 16;
const uint8_t  HEIGHT   = 16*3;
const uint16_t NUM_LEDS = WIDTH * HEIGHT; // 512

// ── Serpentine layout ────────────────────────────────────────
// Works for both panels since HEIGHT is now 32.
inline uint16_t XY(uint8_t x, uint8_t y) {
  if (x >= WIDTH || y >= HEIGHT) return NUM_LEDS; // guard: off-grid
  return (y & 1) ? (y * WIDTH + (WIDTH - 1 - x))
                 : (y * WIDTH + x);
}

// ── Pin → Region map ─────────────────────────────────────────
// 72 global pins (3 panels × 2 MPR per panel × 12 electrodes), each mapped
// to a zone centre on the 16×32 canvas.
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

struct PinSection {
  uint8_t xStart;
  uint8_t yStart;
  uint8_t xEnd;
  uint8_t yEnd;
};

struct PinPoint {
  uint8_t x;
  uint8_t y;
};

struct PinPointGroup {
  PinPoint points[4];
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

  // ── Duplicate map for second panel pair (pins 24-47) ──────────────
  // These are intentionally overlaid so both X/Y fabrics can trigger the
  // same visual regions while you validate hardware and interaction.
  {  1.5f,  2.0f, 1.2f, 1.4f },  // pin 24
  {  5.5f,  2.0f, 1.2f, 1.4f },  // pin 25
  {  9.5f,  2.0f, 1.2f, 1.4f },  // pin 26
  { 13.5f,  2.0f, 1.2f, 1.4f },  // pin 27

  {  1.5f,  7.0f, 1.2f, 1.4f },  // pin 28
  {  5.5f,  7.0f, 1.2f, 1.4f },  // pin 29
  {  9.5f,  7.0f, 1.2f, 1.4f },  // pin 30
  { 13.5f,  7.0f, 1.2f, 1.4f },  // pin 31

  {  1.5f, 12.5f, 1.2f, 1.4f },  // pin 32
  {  5.5f, 12.5f, 1.2f, 1.4f },  // pin 33
  {  9.5f, 12.5f, 1.2f, 1.4f },  // pin 34
  { 13.5f, 12.5f, 1.2f, 1.4f },  // pin 35

  {  1.5f, 18.0f, 1.2f, 1.4f },  // pin 36
  {  5.5f, 18.0f, 1.2f, 1.4f },  // pin 37
  {  9.5f, 18.0f, 1.2f, 1.4f },  // pin 38
  { 13.5f, 18.0f, 1.2f, 1.4f },  // pin 39

  {  1.5f, 23.0f, 1.2f, 1.4f },  // pin 40
  {  5.5f, 23.0f, 1.2f, 1.4f },  // pin 41
  {  9.5f, 23.0f, 1.2f, 1.4f },  // pin 42
  { 13.5f, 23.0f, 1.2f, 1.4f },  // pin 43

  {  1.5f, 28.5f, 1.2f, 1.4f },  // pin 44
  {  5.5f, 28.5f, 1.2f, 1.4f },  // pin 45
  {  9.5f, 28.5f, 1.2f, 1.4f },  // pin 46
  { 13.5f, 28.5f, 1.2f, 1.4f },  // pin 47

  // ── Duplicate map for third panel pair (pins 48-71) ───────────────
  {  1.5f,  2.0f, 1.2f, 1.4f },  // pin 48
  {  5.5f,  2.0f, 1.2f, 1.4f },  // pin 49
  {  9.5f,  2.0f, 1.2f, 1.4f },  // pin 50
  { 13.5f,  2.0f, 1.2f, 1.4f },  // pin 51

  {  1.5f,  7.0f, 1.2f, 1.4f },  // pin 52
  {  5.5f,  7.0f, 1.2f, 1.4f },  // pin 53
  {  9.5f,  7.0f, 1.2f, 1.4f },  // pin 54
  { 13.5f,  7.0f, 1.2f, 1.4f },  // pin 55

  {  1.5f, 12.5f, 1.2f, 1.4f },  // pin 56
  {  5.5f, 12.5f, 1.2f, 1.4f },  // pin 57
  {  9.5f, 12.5f, 1.2f, 1.4f },  // pin 58
  { 13.5f, 12.5f, 1.2f, 1.4f },  // pin 59

  {  1.5f, 18.0f, 1.2f, 1.4f },  // pin 60
  {  5.5f, 18.0f, 1.2f, 1.4f },  // pin 61
  {  9.5f, 18.0f, 1.2f, 1.4f },  // pin 62
  { 13.5f, 18.0f, 1.2f, 1.4f },  // pin 63

  {  1.5f, 23.0f, 1.2f, 1.4f },  // pin 64
  {  5.5f, 23.0f, 1.2f, 1.4f },  // pin 65
  {  9.5f, 23.0f, 1.2f, 1.4f },  // pin 66
  { 13.5f, 23.0f, 1.2f, 1.4f },  // pin 67

  {  1.5f, 28.5f, 1.2f, 1.4f },  // pin 68
  {  5.5f, 28.5f, 1.2f, 1.4f },  // pin 69
  {  9.5f, 28.5f, 1.2f, 1.4f },  // pin 70
  { 13.5f, 28.5f, 1.2f, 1.4f },  // pin 71
};

// ── Pin → fixed LED section helper ──────────────────────────
// Used by SolidColorMode to light a deterministic block per pin for mapping.
// Default section is 2x2 (4 LEDs) centred near each pin's configured region.
inline PinSection getPinSection(uint8_t globalPin) {
  PinSection s = {0, 0, 0, 0};
  if (globalPin >= NUM_PINS) return s;

  const PinRegion& r = PIN_REGIONS[globalPin];

  int xStart = (int)floorf(r.cx);
  int yStart = (int)floorf(r.cy);

  if (xStart < 0) xStart = 0;
  if (yStart < 0) yStart = 0;
  if (xStart > (int)WIDTH - 2) xStart = (int)WIDTH - 2;
  if (yStart > (int)HEIGHT - 2) yStart = (int)HEIGHT - 2;

  s.xStart = (uint8_t)xStart;
  s.yStart = (uint8_t)yStart;
  s.xEnd   = (uint8_t)(xStart + 1);
  s.yEnd   = (uint8_t)(yStart + 1);
  return s;
}

// ── Solid-color calibration helper ──────────────────────────
// Maps one electrode to a 2×2 corner block.
// Electrode 0 -> top-left 2×2 LEDs:
//   (0,0), (1,0), (0,1), (1,1)
// Electrode 1 -> next 2×2 block below:
//   (0,2), (1,2), (0,3), (1,3)
inline PinPointGroup getSolidColorGroup(uint8_t electrode) {
  PinPointGroup g;

  uint8_t yTop = (uint8_t)(electrode * 2);

  if (yTop >= HEIGHT - 1) yTop = HEIGHT - 2;

  g.points[0] = { 0, yTop };
  g.points[1] = { 1, yTop };
  g.points[2] = { 0, (uint8_t)(yTop + 1) };
  g.points[3] = { 1, (uint8_t)(yTop + 1) };
  return g;
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
