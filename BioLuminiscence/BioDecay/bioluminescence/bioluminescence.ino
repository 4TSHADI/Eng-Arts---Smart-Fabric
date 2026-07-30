/*
  Touch coordinate test

  This sketch reads the real MPR121 touch electrodes and prints the
  live touched point as (x, y), where:
    - x uses pins 0..5
    - y uses pins 6..11

  Example:
    X pin 2 and Y pin 8 -> (x=2, y=2)
*/

#include <Wire.h>
#include "Adafruit_MPR121.h"
#include "ProjectConfig.h"

Adafruit_MPR121 caps[NUM_MUX_CHANNELS];
bool mprReady[NUM_MUX_CHANNELS] = {false};
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

static const uint8_t X_CHANNELS[2] = {0, 1};
static const uint8_t Y_CHANNELS[2] = {2, 3};

struct TouchResult {
  bool valid;
  int8_t pin;
  int16_t pressure;
};

static const uint8_t X_PIN_MIN = 0;
static const uint8_t X_PIN_MAX = 5;
static const uint8_t Y_PIN_MIN = 6;
static const uint8_t Y_PIN_MAX = 11;

void tcaSelect(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(TCA9548A_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  strip.begin();
  strip.clear();
  strip.show();

  Serial.println("Touch + LED coordinate test ready");
  Serial.println("Panel 0 uses X ch0 + Y ch2, panel 1 uses X ch1 + Y ch3");

  for (uint8_t ch = 0; ch < 4; ++ch) {
    tcaSelect(ch);
    if (caps[ch].begin(MPR121_ADDR)) {
      caps[ch].setAutoconfig(true);
      caps[ch].setThresholds(12, 6);
      mprReady[ch] = true;
      Serial.print("MPR121 channel ");
      Serial.print(ch);
      Serial.println(" found");
    } else {
      Serial.print("MPR121 channel ");
      Serial.print(ch);
      Serial.println(" not found");
    }
  }
}

bool panelReady(uint8_t panel) {
  return mprReady[X_CHANNELS[panel]] && mprReady[Y_CHANNELS[panel]];
}

uint8_t axisCellToPixel(uint8_t axisCell) {
  // Scale 6 cells (0..5) onto panel pixels (0..15).
  return (uint8_t)((axisCell * 15) / 5);
}

uint8_t mapXPinToPixel(uint8_t pin) {
  // User layout is right-to-left for X labels: 0,1,2,3,4,5.
  uint8_t axisCell = (uint8_t)(pin - X_PIN_MIN);
  return axisCellToPixel((uint8_t)(5 - axisCell));
}

uint8_t mapYPinToPixel(uint8_t pin) {
  // User layout is top-to-bottom for Y labels: 6,7,8,9,10,11.
  uint8_t axisCell = (uint8_t)(pin - Y_PIN_MIN);
  return axisCellToPixel(axisCell);
}

void drawPoint(uint8_t x, uint8_t y, uint8_t brightness) {
  if (x >= WIDTH || y >= HEIGHT) return;
  uint16_t idx = XY(x, y);
  if (idx >= NUM_LEDS) return;
  strip.setPixelColor(idx, bioColor(strip, brightness));
}

TouchResult getStrongestTouch(Adafruit_MPR121& sensor,
                              uint8_t muxChannel,
                              uint8_t pinMin,
                              uint8_t pinMax) {
  tcaSelect(muxChannel);
  delay(2);

  TouchResult result = {false, -1, -1};
  uint16_t touched = sensor.touched();

  for (uint8_t i = pinMin; i <= pinMax; ++i) {
    if (!(touched & (1 << i))) continue;

    int16_t pressure = (int16_t)sensor.baselineData(i) - (int16_t)sensor.filteredData(i);
    if (pressure < 0) pressure = 0;

    if (!result.valid || pressure > result.pressure) {
      result.valid = true;
      result.pin = (int8_t)i;
      result.pressure = pressure;
    }
  }

  return result;
}

void loop() {
  strip.clear();

  bool anyPoint = false;

  for (uint8_t panel = 0; panel < 2; ++panel) {
    if (!panelReady(panel)) continue;

    uint8_t xCh = X_CHANNELS[panel];
    uint8_t yCh = Y_CHANNELS[panel];

    TouchResult xTouch = getStrongestTouch(caps[xCh], xCh, X_PIN_MIN, X_PIN_MAX);
    TouchResult yTouch = getStrongestTouch(caps[yCh], yCh, Y_PIN_MIN, Y_PIN_MAX);

    if (!xTouch.valid || !yTouch.valid) continue;

    uint8_t x = mapXPinToPixel((uint8_t)xTouch.pin);
    uint8_t yLocal = mapYPinToPixel((uint8_t)yTouch.pin);
    uint8_t y = (uint8_t)(panel * 16 + yLocal);

    int16_t strength = (xTouch.pressure + yTouch.pressure) / 2;
    if (strength > 255) strength = 255;
    if (strength < 40) strength = 40;

    drawPoint(x, y, (uint8_t)strength);
    anyPoint = true;

    Serial.print("Panel ");
    Serial.print(panel);
    Serial.print(" -> (x=");
    Serial.print(x);
    Serial.print(", y=");
    Serial.print(y);
    Serial.print(") using ch");
    Serial.print(xCh);
    Serial.print("/ch");
    Serial.println(yCh);
  }

  if (!anyPoint) {
  }

  strip.show();
  delay(50);
}
