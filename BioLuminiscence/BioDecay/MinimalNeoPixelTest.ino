#include <Adafruit_NeoPixel.h>

#define LED_PIN 6
#define NUM_LEDS 512

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_BGR + NEO_KHZ800);

void setup() {
  Serial.begin(9600);
  while (!Serial) { delay(10); }

  strip.begin();
  strip.setBrightness(255);
  strip.clear();
  strip.show();

  Serial.println("Testing NeoPixel on pin 6...");
  for (int i = 0; i < 3; i++) {
    strip.fill(strip.Color(0, 255, 0));
    strip.show();
    delay(500);
    strip.clear();
    strip.show();
    delay(500);
  }
}

void loop() {
  strip.fill(strip.Color(0, 0, 255));
  strip.show();
  delay(500);
  strip.clear();
  strip.show();
  delay(500);
}
