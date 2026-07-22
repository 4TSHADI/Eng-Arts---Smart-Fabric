#include <FastLED.h>

// Matrix dimensions
#define MATRIX_WIDTH  13
#define MATRIX_HEIGHT 13
#define NUM_LEDS      (MATRIX_WIDTH * MATRIX_HEIGHT)

#define LED_PIN       6     // Pin connected to NeoPixel Data In
#define COLOR_ORDER   GRB
#define LED_TYPE      WS2812B
#define MAX_BRIGHTNESS 120   // Keep brightness moderate for USB power limits (Uno)

CRGB leds[NUM_LEDS];

// Heat map memory array (takes 169 bytes of SRAM)
uint8_t heat[MATRIX_WIDTH][MATRIX_HEIGHT];

// XY mapping function for standard Serpentine layout
uint16_t getLEDIndex(uint8_t x, uint8_t y) {
  if (x >= MATRIX_WIDTH || y >= MATRIX_HEIGHT) return 0;
  if (y % 2 == 0) {
    return (y * MATRIX_WIDTH) + x;
  } else {
    return (y * MATRIX_WIDTH) + (MATRIX_WIDTH - 1 - x);
  }
}

// Touch Event structure
struct TouchEvent {
  bool active;
  uint8_t x;
  uint8_t y;
  uint8_t intensity;
};

// Touch Check function (with simulated mock touch inputs)
TouchEvent checkTouch() {
  TouchEvent event;
  event.active = false;
  
  if (random8() < 4) { // ~4% chance to simulate a touch
    event.active = true;
    event.x = random8(MATRIX_WIDTH);
    event.y = random8(MATRIX_HEIGHT);
    event.intensity = random8(120, 255);
  }
  return event;
}

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(MAX_BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
  randomSeed(analogRead(0));

  // Initialize heatmap to 0
  memset(heat, 0, sizeof(heat));
}

void loop() {
  TouchEvent touch = checkTouch();

  // 1. Cool down the heatmap
  for (int x = 0; x < MATRIX_WIDTH; x++) {
    for (int y = 0; y < MATRIX_HEIGHT; y++) {
      uint8_t cooldown = random8(1, 3);
      if (heat[x][y] > cooldown) {
        heat[x][y] -= cooldown;
      } else {
        heat[x][y] = 0;
      }
    }
  }

  // 2. Add heat at the touch point
  if (touch.active) {
    uint8_t radius = 3;
    for (int8_t dx = -radius; dx <= radius; dx++) {
      for (int8_t dy = -radius; dy <= radius; dy++) {
        int nx = touch.x + dx;
        int ny = touch.y + dy;
        
        if (nx >= 0 && nx < MATRIX_WIDTH && ny >= 0 && ny < MATRIX_HEIGHT) {
          // Heat intensity drops off with distance from center
          int distSquared = dx*dx + dy*dy;
          if (distSquared < radius*radius) {
            uint16_t heatAdd = (radius - sqrt(distSquared)) * (touch.intensity / 3);
            if (heat[nx][ny] + heatAdd > 255) {
              heat[nx][ny] = 255;
            } else {
              heat[nx][ny] += heatAdd;
            }
          }
        }
      }
    }
  }

  // 3. Map heat levels to thermal colors (Black -> Blue -> Red -> Orange -> Yellow -> White)
  for (int x = 0; x < MATRIX_WIDTH; x++) {
    for (int y = 0; y < MATRIX_HEIGHT; y++) {
      uint8_t hVal = heat[x][y];
      uint16_t idx = getLEDIndex(x, y);

      if (hVal == 0) {
        leds[idx] = CRGB::Black;
      } else if (hVal < 64) {
        // Cool blue to purple
        leds[idx] = CRGB(0, 0, hVal * 4);
      } else if (hVal < 128) {
        // Purple to bright red
        leds[idx] = CRGB((hVal - 64) * 4, 0, 255 - ((hVal - 64) * 4));
      } else if (hVal < 192) {
        // Red to orange-yellow
        leds[idx] = CRGB(255, (hVal - 128) * 4, 0);
      } else {
        // Yellow to white hot
        leds[idx] = CRGB(255, 255, (hVal - 192) * 4);
      }
    }
  }

  FastLED.show();
  FastLED.delay(20); // ~50 FPS
}
