/*********************************************************
This is a library for the MPR121 12-channel Capacitive touch sensor

Designed specifically to work with the MPR121 Breakout in the Adafruit shop
  ----> https://www.adafruit.com/products/

These sensors use I2C communicate, at least 2 pins are required
to interface

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada for Adafruit Industries.
BSD license, all text above must be included in any redistribution
**********************************************************/

#include <Wire.h>
#include "Adafruit_MPR121.h"

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

Adafruit_MPR121 cap = Adafruit_MPR121();

uint16_t lasttouched = 0;
uint16_t currtouched = 0;

#define MY_PAD 0  // <-- Change this to whichever pad you are using (0 to 11)

// -------------------------------------------------------
// YOUR TOUCH FUNCTION
// -------------------------------------------------------
void onTouched() {
  Serial.println("Pad touched! Doing something...");
  // === ADD YOUR CODE HERE ===
  // e.g. digitalWrite(LED_PIN, HIGH);
}

// -------------------------------------------------------
// YOUR RELEASE FUNCTION
// -------------------------------------------------------
void onReleased() {
  Serial.println("Pad released! Doing something...");
  // === ADD YOUR CODE HERE ===
  // e.g. digitalWrite(LED_PIN, LOW);
}

// -------------------------------------------------------
// SETUP
// -------------------------------------------------------
void setup() {
  Serial.begin(9600);

  while (!Serial) {
    delay(10);
  }

  Serial.println("Adafruit MPR121 Capacitive Touch sensor test");

  if (!cap.begin(0x5B)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
  Serial.println("MPR121 found!");

  Serial.println("Running auto configuration.");
  cap.setAutoconfig(true);

  Serial.println("Initialization complete.");
}

// -------------------------------------------------------
// MAIN LOOP
// -------------------------------------------------------
void loop() {
  currtouched = cap.touched();

  // Check only MY_PAD
  if ((currtouched & _BV(MY_PAD)) && !(lasttouched & _BV(MY_PAD))) {
    onTouched();   // Pad was just touched
        delay(10);

  }

  if (!(currtouched & _BV(MY_PAD)) && (lasttouched & _BV(MY_PAD))) {
    onReleased();  // Pad was just released
  }

  lasttouched = currtouched;
}