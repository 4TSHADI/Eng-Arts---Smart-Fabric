#ifndef PROJECT_CONFIG_H
#define PROJECT_CONFIG_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <unordered_map>

const int WIDTH = 16;
const int HEIGHT = 16;
const int NUM_LEDS = 256;

// Nano/Uno-compatible data pin for the NeoPixel strip.
const uint8_t LED_PIN = 6;

// MPR121 I2C address.
const uint8_t MPR121_ADDR = 0x5B;

struct Point {
    uint8_t x;
    uint8_t y;
};

extern const Point centerCoordinates[12];
// Coordinate mapping helper
uint16_t XY(uint8_t x, uint8_t y);

// Region boundary definition
struct Region {
  uint8_t xStart;
  uint8_t xEnd;
  uint8_t yStart;
  uint8_t yEnd;
};

// 12 regions covering the 16x16 grid (4 columns x 3 rows)
extern const Region regions[12];

#endif
