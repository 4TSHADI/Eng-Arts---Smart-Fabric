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

// XY mapping function for standard Serpentine layout
// (First row: left-to-right, second row: right-to-left, etc.)
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

// Touch Check function
TouchEvent checkTouch() {
  TouchEvent event;
  event.active = false;
  
  // --- MOCK SIMULATION ---
  // If you don't have a sensor wired, this randomly simulates touches.
  // Replace this with your actual sensor reading (e.g., analogRead or digitalRead).
  if (random8() < 6) { // ~6% chance per frame to generate a touch
    event.active = true;
    event.x = random8(MATRIX_WIDTH);
    event.y = random8(MATRIX_HEIGHT);
    event.intensity = random8(150, 255);
  }
  return event;
}

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(MAX_BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
  randomSeed(analogRead(0)); // Seed random generator with floating analog pin
}

void loop() {
  TouchEvent touch = checkTouch();

  // 1. Slow decay - fades the trail to create the bioluminescent wake
  fadeToBlackBy(leds, NUM_LEDS, 15); 

  // 2. If touch is active, draw a small glowing blue/teal brush
  if (touch.active) {
    uint8_t radius = 2; // Brush size
    for (int8_t dx = -radius; dx <= radius; dx++) {
      for (int8_t dy = -radius; dy <= radius; dy++) {
        int nx = touch.x + dx;
        int ny = touch.y + dy;

        // Circular bounding check
        if (nx >= 0 && nx < MATRIX_WIDTH && ny >= 0 && ny < MATRIX_HEIGHT) {
          if (dx*dx + dy*dy <= radius*radius) {
            uint16_t idx = getLEDIndex(nx, ny);
            // Neon cyan/teal/blue range: HSV hues 130 to 160
            uint8_t hue = 130 + random8(30); 
            leds[idx] = CHSV(hue, 255, touch.intensity);
          }
        }
      }
    }
  }

  FastLED.show();
  FastLED.delay(20); // ~50 FPS
}

