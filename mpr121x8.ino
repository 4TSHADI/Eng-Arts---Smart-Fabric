#include <Wire.h>
#include "Adafruit_MPR121.h"

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

#define TCA_ADDR    0x70
#define NUM_SENSORS 8

Adafruit_MPR121 cap[NUM_SENSORS];
uint16_t lasttouched[NUM_SENSORS] = {0};
uint16_t currtouched[NUM_SENSORS] = {0};

void tcaSelect(uint8_t channel) {
  Wire.beginTransmission(TCA_ADDR);
  Wire.write((channel > 7) ? 0 : (1 << channel));
  Wire.endTransmission();
}

void setup() {
  Serial.begin(9600);
  while (!Serial) { delay(10); }

  Wire.begin();
  delay(100);

  Serial.println("=== Initializing 8x MPR121 via TCA9548A ===");

  for (uint8_t s = 0; s < NUM_SENSORS; s++) {
    tcaSelect(s);
    delay(10);
    if (!cap[s].begin(0x5A)) {
      Serial.print("ERROR: MPR121 #");
      Serial.print(s);
      Serial.println(" not found!");
      while (1);
    }
    Serial.print("MPR121 #");
    Serial.print(s);
    Serial.println(" found ✓");
  }

  tcaSelect(255);
  Serial.println("=== All 8 sensors ready ===\n");
}

void loop() {
  for (uint8_t s = 0; s < NUM_SENSORS; s++) {
    tcaSelect(s);
    currtouched[s] = cap[s].touched();

    for (uint8_t pad = 0; pad < 12; pad++) {
      if ((currtouched[s] & _BV(pad)) && !(lasttouched[s] & _BV(pad))) {
        Serial.print("Sensor "); Serial.print(s);
        Serial.print(" | Pad "); Serial.print(pad);
        Serial.println(" → TOUCHED");
      }
      if (!(currtouched[s] & _BV(pad)) && (lasttouched[s] & _BV(pad))) {
        Serial.print("Sensor "); Serial.print(s);
        Serial.print(" | Pad "); Serial.print(pad);
        Serial.println(" → released");
      }
    }
    lasttouched[s] = currtouched[s];
  }
  delay(10);
}
