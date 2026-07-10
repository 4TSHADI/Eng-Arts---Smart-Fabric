#include <Wire.h>
#include "Adafruit_MPR121.h"

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

// You can have up to 4 on one i2c bus but one is enough for testing!
Adafruit_MPR121 cap = Adafruit_MPR121();

// Keeps track of the last pins touched
// so we know when buttons are 'released'
uint16_t lasttouched = 0;
uint16_t currtouched = 0;

bool mprReady = false;
const uint8_t MPR121_ADDR = 0x5B;
uint32_t lastHealthCheck = 0;
const uint32_t HEALTH_CHECK_INTERVAL_MS = 3000; // probe the bus every 3s

void setup() {
  Serial.begin(9600);

  while (!Serial) { // needed to keep leonardo/micro from starting too fast!
    delay(10);
  }

  Serial.println("Adafruit MPR121 Capacitive Touch sensor test");

  Wire.begin();
  // If the bus ever gets stuck (a device holds SDA/SCL low mid-transaction),
  // this forces the Wire hardware to reset itself after 25ms instead of
  // hanging the whole sketch forever.
  Wire.setWireTimeout(25000, true);

  tryInitMPR121();
}

void tryInitMPR121() {
  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(MPR121_ADDR)) {
    Serial.println("MPR121 not found, check wiring? Will retry...");
    mprReady = false;
    return;
  }
  Serial.println("MPR121 found!");

  Serial.println("Running auto configuration.");
  cap.setAutoconfig(true);

  Serial.println("Initialization complete.");
  mprReady = true;
}

void loop() {
  // If init failed (or the bus locked up and we detected it), keep retrying
  // instead of freezing forever - this avoids needing a manual power cycle
  // every time the bus hiccups.
  if (!mprReady) {
    delay(500);
    tryInitMPR121();
    return;
  }

  // Periodically probe the bus directly (cheap: just an address ack, no
  // data). If the MPR121 stops acknowledging - e.g. after a loose-wire glitch
  // or a bus lockup that setWireTimeout recovered from - this catches it and
  // triggers a clean re-init instead of silently returning stale/garbage data.
  uint32_t now = millis();
  if (now - lastHealthCheck > HEALTH_CHECK_INTERVAL_MS) {
    lastHealthCheck = now;
    Wire.beginTransmission(MPR121_ADDR);
    uint8_t err = Wire.endTransmission();
    if (err != 0) {
      Serial.println("I2C health check failed - lost MPR121, re-initializing.");
      mprReady = false;
      return;
    }
  }

  // Get the currently touched pads
  currtouched = cap.touched();

  for (uint8_t i = 0; i < 12; i++) {
    // it if *is* touched and *wasnt* touched before, alert!
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i))) {
      Serial.print(i); Serial.println(" touched");
    }
    // if it *was* touched and now *isnt*, alert!
    if (!(currtouched & _BV(i)) && (lasttouched & _BV(i))) {
      Serial.print(i); Serial.println(" released");
    }
  }

  // reset our state
  lasttouched = currtouched;

  delay(100); // put a delay so it isn't overwhelming
}
