#include "bioluminescentMode.h"

int radius = 1; 

void BioluminescentMode::enter(Adafruit_NeoPixel& strip) {
  strip.setBrightness(50); // Set standard brightness
  strip.clear();
  strip.show();
}

void safeSetPixel(int x, int y, uint32_t color, Adafruit_NeoPixel& strip) {
  if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
    uint16_t idx = XY(x, y);
    if (idx < NUM_LEDS) {
      strip.setPixelColor(idx, color);
    }
  }
}

uint8_t lerp(uint8_t startVal, uint8_t endVal, float t) {
  return startVal + t * (endVal - startVal);
}



int getWidthAt(int dx, int headRadius, int length) {
  int absDx = abs(dx);
  
  if (absDx <= headRadius) {
    return absDx;
  } else {
    float t = (float)(absDx - headRadius) / (length - headRadius);
    int dy_max = headRadius * (1.0 - t);
    return dy_max;
  }
}

void drawTrail(int tipX, int tipY, int headRadius, int length, Adafruit_NeoPixel& strip) {
  int headCenterX = -headRadius;

  for (int dx = 0; dx >= -length; dx--) {
    int dy_max = getWidthAt(dx, headRadius, length);
    
    for (int dy = -dy_max; dy <= dy_max; dy++) {
      
      uint32_t color = strip.Color(100,8,9);
      safeSetPixel(tipX + dx, tipY + dy, color, strip);
  } }}

void BioluminescentMode::onTouch(Adafruit_NeoPixel& strip, uint8_t pin, bool isTouched, int16_t pressure) {
  if (pin >= 12) return;
  Serial.print("Pressure "); Serial.print(pressure);
  uint8_t length = radius*6;
  for (int i = -radius; i < WIDTH + length; i++) {
    strip.clear();
    
    drawTrail(i, 8, radius, length, strip);
    
    strip.show();
    delay(100);
  }

  strip.show();
}

void BioluminescentMode::update(Adafruit_NeoPixel& strip) {
  // Static placeholder color mode doesn't need temporal updates
}
