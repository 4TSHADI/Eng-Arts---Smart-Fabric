#include <Wire.h>
#include "Adafruit_MPR121.h"

#define TCA9548A_ADDR 0x70
#define NUM_MUX_CHANNELS 6
#define MPR121_ADDR 0x5A

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

Adafruit_MPR121 cap[NUM_MUX_CHANNELS];
bool mprReady[NUM_MUX_CHANNELS];

uint16_t lasttouched[NUM_MUX_CHANNELS];
uint16_t currtouched[NUM_MUX_CHANNELS];

void tcaSelect(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(TCA9548A_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
}
void setup() {
  Serial.begin(9600);
  while (!Serial) { delay(10); }

  Wire.begin();

  Serial.println("Initializing 8x MPR121 via TCA9548A...");

  for (uint8_t ch = 0; ch < NUM_MUX_CHANNELS; ch++) {
    tcaSelect(ch);
    if (cap[ch].begin(MPR121_ADDR)) {
      cap[ch].setAutoconfig(true);
      cap[ch].setThresholds(40, 20);

      Serial.print("Channel "); Serial.print(ch); Serial.println(": MPR121 found.");
      mprReady[ch] = true;
    } else {
      Serial.print("Channel "); Serial.print(ch); Serial.println(": MPR121 NOT found.");
      mprReady[ch] = false;
    }
    lasttouched[ch] = 0;
  }

  Serial.println("Setup complete.");
}

void loop() {
  for (uint8_t ch = 0; ch < NUM_MUX_CHANNELS; ch++) {
    if (!mprReady[ch]) continue;

    tcaSelect(ch);
    currtouched[ch] = cap[ch].touched();

    for (uint8_t i = 0; i < 12; i++) {
      if ((currtouched[ch] & _BV(i)) && !(lasttouched[ch] & _BV(i))) {
        Serial.print("Ch "); Serial.print(ch);
        Serial.print(" - electrode "); Serial.print(i);
        Serial.println(" touched");
      }
      if (!(currtouched[ch] & _BV(i)) && (lasttouched[ch] & _BV(i))) {
        Serial.print("Ch "); Serial.print(ch);
        Serial.print(" - electrode "); Serial.print(i);
        Serial.println(" released");
      }
    }

    lasttouched[ch] = currtouched[ch];
  }

  delay(50);
}
