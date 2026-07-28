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

void tcaSelect(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(TCA9548A_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

void setup() {
  Serial.begin(9600);
  Wire.begin();

  Serial.println("Touch coordinate test ready");
  Serial.println("Waiting for MPR121 touch input...");

  for (uint8_t ch = 0; ch < 2; ++ch) {
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

void printDetectedPoint() {
  if (!mprReady[0] || !mprReady[1]) {
    Serial.println("MPR121 sensors not ready");
    return;
  }

  tcaSelect(0);
  uint16_t touchedX = caps[0].touched();

  tcaSelect(1);
  uint16_t touchedY = caps[1].touched();

  int maxPinX = -1;
  int16_t maxPressureX = -1;
  tcaSelect(0);
  for (uint8_t i = 0; i < 12; ++i) {
    if (touchedX & (1 << i)) {
      int16_t pressure = (int16_t)caps[0].baselineData(i) - (int16_t)caps[0].filteredData(i);
      if (pressure > maxPressureX) {
        maxPressureX = pressure;
        maxPinX = i;
      }
    }
  }

  int maxPinY = -1;
  int16_t maxPressureY = -1;
  tcaSelect(1);
  for (uint8_t i = 0; i < 12; ++i) {
    if (touchedY & (1 << i)) {
      int16_t pressure = (int16_t)caps[1].baselineData(i) - (int16_t)caps[1].filteredData(i);
      if (pressure > maxPressureY) {
        maxPressureY = pressure;
        maxPinY = i;
      }
    }
  }

  if (maxPinX < 0 || maxPinY < 0) {
    Serial.println("No touch detected");
    return;
  }

  int x = maxPinX;
  int y = maxPinY - 6;

  Serial.print("Touched point -> (x=");
  Serial.print(x);
  Serial.print(", y=");
  Serial.print(y);
  Serial.println(")");
}

void loop() {
  printDetectedPoint();
  delay(50);
}
