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

// Splash data structure
struct Splash {
  int x;
  int y;
  CRGB color;
  uint8_t radius;
  uint8_t maxRadius;
  bool active;
};

// Keep splashes to 3 maximum to conserve SRAM on Arduino Uno
#define MAX_SPLASHES 3
Splash splashes[MAX_SPLASHES];

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
};

// Touch Check function (simulating a tap)
TouchEvent checkTouch() {
  TouchEvent event;
  event.active = false;
  
  if (random8() < 3) { // ~3% chance per frame to register a splash
    event.active = true;
    event.x = random8(MATRIX_WIDTH);
    event.y = random8(MATRIX_HEIGHT);
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

  // Initialize splashes as inactive
  for (int i = 0; i < MAX_SPLASHES; i++) {
    splashes[i].active = false;
  }
}

void loop() {
  TouchEvent touch = checkTouch();

  // Fades the background quickly so new splashes pop out
  fadeToBlackBy(leds, NUM_LEDS, 35);

  // 1. If touched, trigger a new splash
  if (touch.active) {
    for (int i = 0; i < MAX_SPLASHES; i++) {
      if (!splashes[i].active) {
        splashes[i].active = true;
        splashes[i].x = touch.x;
        splashes[i].y = touch.y;
        splashes[i].radius = 1;
        splashes[i].maxRadius = random8(3, 6); // Max size of the splatter
        splashes[i].color = CHSV(random8(), 255, 255); // Random HSV color
        break;
      }
    }
  }

  // 2. Animate and draw splashes
  for (int i = 0; i < MAX_SPLASHES; i++) {
    if (splashes[i].active) {
      // Draw splash using noise-modulated boundaries for a dynamic, uneven shape
      for (int8_t dx = -splashes[i].radius; dx <= splashes[i].radius; dx++) {
        for (int8_t dy = -splashes[i].radius; dy <= splashes[i].radius; dy++) {
          int nx = splashes[i].x + dx;
          int ny = splashes[i].y + dy;

          if (nx >= 0 && nx < MATRIX_WIDTH && ny >= 0 && ny < MATRIX_HEIGHT) {
            // Use FastLED's fast inoise8 to simulate splatter shape irregularities
            uint8_t noiseVal = inoise8(nx * 120, ny * 120, millis() / 15);
            float noiseFactor = 0.5 + (noiseVal / 255.0); // Modulates the radius boundaries
            
            if (dx*dx + dy*dy <= (splashes[i].radius * noiseFactor) * (splashes[i].radius * noiseFactor)) {
              uint16_t idx = getLEDIndex(nx, ny);
              leds[idx] = splashes[i].color;
            }
          }
        }
      }

      // Grow the splash radius slowly
      if (random8() < 120) {
        splashes[i].radius++;
      }

      // Deactivate/Drip when full size is reached
      if (splashes[i].radius >= splashes[i].maxRadius) {
        // Drip gravity logic: splash center drips downwards before deactivating
        if (splashes[i].y < MATRIX_HEIGHT - 1 && random8() < 90) {
          splashes[i].y++; // Move the splatter center down
        } else {
          splashes[i].active = false; // Turn off splash
        }
      }
    }
  }

  FastLED.show();
  FastLED.delay(25); // ~40 FPS
}
