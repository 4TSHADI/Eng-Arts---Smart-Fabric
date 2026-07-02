#include <Adafruit_NeoPixel.h>

#define LED_PIN     6
#define NUM_LEDS    60

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_BGR + NEO_KHZ800);

void setup() {
  strip.begin();           
  strip.show();            
  strip.setBrightness(50); 
}

void loop() {
  // Example: Turn all LEDs red
  for(int i=0; i<NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(255, 0, 0));
  }
  strip.show();
}
