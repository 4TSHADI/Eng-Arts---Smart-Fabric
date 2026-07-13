#include "ProjectConfig.h"

// 12 regions mapped to a 16x16 matrix (4 columns, 3 rows)
const Region regions[12] = {
  // Row 0 (Top)
  {0, 3, 0, 4},   // Region 0
  {4, 7, 0, 4},   // Region 1
  {8, 11, 0, 4},  // Region 2
  {12, 15, 0, 4}, // Region 3

  // Row 1 (Middle)
  {0, 3, 5, 9},   // Region 4
  {4, 7, 5, 9},   // Region 5
  {8, 11, 5, 9},  // Region 6
  {12, 15, 5, 9}, // Region 7

  // Row 2 (Bottom)
  {0, 3, 10, 15},  // Region 8
  {4, 7, 10, 15},  // Region 9
  {8, 11, 10, 15}, // Region 10
  {12, 15, 10, 15} // Region 11
};

// XY mapping function for standard Serpentine layout (zig-zag grid)
uint16_t XY(uint8_t x, uint8_t y) {
  if (x >= WIDTH || y >= HEIGHT) return NUM_LEDS; // Return out of bounds marker

  if (y & 1) {
    // Odd row: right to left
    return y * WIDTH + (WIDTH - 1 - x);
  } else {
    // Even row: left to right
    return y * WIDTH + x;
  }
}

const Point centerCoordinates[12] = {
    {2, 1},
    {2, 5},
    {2, 9},
    {2, 13},
    {7, 1},
    {7, 5},
    {7, 9},
    {7, 13},
    {13, 1},
    {13, 5},
    {13, 9},
    {13, 13}
};