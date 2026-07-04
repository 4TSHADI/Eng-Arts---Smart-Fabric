#include <Wire.h>
#include "Adafruit_MPR121.h"

#define TCA_ADDR 0x70
#define NUM_SENSORS 8  

Adafruit_MPR121 cap[NUM_SENSORS];
uint16_t lasttouched[NUM_SENSORS] = {0};
uint16_t currtouched[NUM_SENSORS] = {0};

// Switch TCA9548A to the specified channel (0-7)
void tcaSelect(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(TCA_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

void setup() {
  Serial.begin(9600);
  while (!Serial) { delay(10); }
  Wire.begin();

  Serial.println("Initializing MPR121 sensors via TCA9548A...");

  for (uint8_t i = 0; i < NUM_SENSORS; i++) {
    tcaSelect(i);
    if (!cap[i].begin(0x5A)) {
      Serial.print("MPR121 #"); Serial.print(i);
      Serial.println(" not found, check wiring?");
    } else {
      Serial.print("MPR121 #"); Serial.print(i);
      Serial.println(" found!");
    }
  }

  Serial.println("Initialization complete.");
}

void loop() {
  for (uint8_t s = 0; s < NUM_SENSORS; s++) {
    tcaSelect(s);
    currtouched[s] = cap[s].touched();

    for (uint8_t i = 0; i < 12; i++) {
      // Pad just touched
      if ((currtouched[s] & (1 << i)) && !(lasttouched[s] & (1 << i))) {
        Serial.print("Sensor "); Serial.print(s);
        Serial.print(" Pad "); Serial.print(i);
        Serial.println(" touched");
      }
      // Pad just released
      if (!(currtouched[s] & (1 << i)) && (lasttouched[s] & (1 << i))) {
        Serial.print("Sensor "); Serial.print(s);
        Serial.print(" Pad "); Serial.print(i);
        Serial.println(" released");
      }
    }
    lasttouched[s] = currtouched[s];
  }
  delay(50);
}