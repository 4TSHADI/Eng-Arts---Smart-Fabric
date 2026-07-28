#ifndef PROJECT_CONFIG_H
#define PROJECT_CONFIG_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

const int WIDTH = 16;
const int HEIGHT = 16;
const int NUM_LEDS = 256;
const uint8_t NUM_MUX_CHANNELS = 6;

struct Region {
  uint8_t xStart;
  uint8_t xEnd;
  uint8_t yStart;
  uint8_t yEnd;
};

// 6 regions mapped to a 16x16 grid
const Region regions[NUM_MUX_CHANNELS] = {
  {1, 4, 3, 8},    // Region 0: Green (Top-Left)
  {6, 9, 3, 8},    // Region 1: Orange (Top-Middle)
  {11, 14, 3, 8},  // Region 2: Red (Top-Right)
  {1, 4, 10, 15},   // Region 3: Blue (Bottom-Left)
  {6, 9, 10, 15},   // Region 4: Yellow (Bottom-Middle)
  {11, 14, 10, 15}  // Region 5: Purple (Bottom-Right)
};

// Serpentine layout coordinate mapping
inline uint16_t XY(uint8_t x, uint8_t y) {
  if (x >= WIDTH || y >= HEIGHT) return NUM_LEDS;
  if (y & 1) {
    return y * WIDTH + (WIDTH - 1 - x);
  } else {
    return y * WIDTH + x;
  }
}

// Region colors
inline uint32_t getRegionColor(Adafruit_NeoPixel& strip, uint8_t ch) {
  switch (ch) {
    case 0: return strip.Color(128, 255, 0);   // Green
    case 1: return strip.Color(255, 127, 0);   // Orange
    case 2: return strip.Color(255, 0, 0);     // Red
    case 3: return strip.Color(0, 191, 255);   // Blue 
    case 4: return strip.Color(255, 255, 0);   // Yellow
    case 5: return strip.Color(127, 0, 255);   // Purple
    default: return strip.Color(255, 255, 255);
  }
}

// Get XY coordinates inside a region for a given pin offset
inline void getRegionPixel(uint8_t ch, uint8_t offset, uint8_t &outX, uint8_t &outY) {
  Region r = regions[ch];
  uint8_t row = offset / 4;
  uint8_t col = offset % 4;
  outX = r.xStart + col;
  outY = r.yStart + row;
}

#endif
