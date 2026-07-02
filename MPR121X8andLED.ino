
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

// -------------------------------------------------------
// FUNCTION CALLED WHEN A PAD IS TOUCHED
// -------------------------------------------------------
void onTouched(uint8_t padNumber) {
  Serial.print("Pad ");
  Serial.print(padNumber);
  Serial.println(" touched → running action...");

  switch (padNumber) {
    case 0:
      // Action for pad 0
      Serial.println("→ Pad 0 action: e.g. Turn ON LED");
      // digitalWrite(LED_PIN, HIGH);
      break;

    case 1:
      // Action for pad 1
      Serial.println("→ Pad 1 action: e.g. Play a tone");
      // tone(BUZZER_PIN, 440, 200);
      break;

    case 2:
      // Action for pad 2
      Serial.println("→ Pad 2 action: e.g. Trigger a relay");
      // digitalWrite(RELAY_PIN, HIGH);
      break;

    case 3:
      Serial.println("→ Pad 3 action");
      // your code here
      break;

    case 4:
      Serial.println("→ Pad 4 action");
      // your code here
      break;

    case 5:
      Serial.println("→ Pad 5 action");
      // your code here
      break;

    case 6:
      Serial.println("→ Pad 6 action");
      // your code here
      break;

    case 7:
      Serial.println("→ Pad 7 action");
      // your code here
      break;

    case 8:
      Serial.println("→ Pad 8 action");
      // your code here
      break;

    case 9:
      Serial.println("→ Pad 9 action");
      // your code here
      break;

    case 10:
      Serial.println("→ Pad 10 action");
      // your code here
      break;

    case 11:
      Serial.println("→ Pad 11 action");
      // your code here
      break;

    default:
      Serial.println("→ Unknown pad, no action assigned.");
      break;
  }
}

// -------------------------------------------------------
// FUNCTION CALLED WHEN A PAD IS RELEASED
// -------------------------------------------------------
void onReleased(uint8_t padNumber) {
  Serial.print("Pad ");
  Serial.print(padNumber);
  Serial.println(" released → running release action...");

  switch (padNumber) {
    case 0:
      // Release action for pad 0
      // digitalWrite(LED_PIN, LOW);
      break;

    case 1:
      // Release action for pad 1
      // noTone(BUZZER_PIN);
      break;

    // Add release actions for other pads as needed...

    default:
      break;
  }
}

// -------------------------------------------------------
// SETUP
// -------------------------------------------------------
void setup() {
  Serial.begin(9600);

  while (!Serial) { // needed to keep leonardo/micro from starting too fast!
    delay(10);
  }

  Serial.println("Adafruit MPR121 Capacitive Touch sensor test");

  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5B)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
  Serial.println("MPR121 found!");

  // This is generally recommended since it seems to work well for most setups.
  // Can remove if wanting to manually configure touch channels (CDC and CDT).
  Serial.println("Running auto configuration.");
  cap.setAutoconfig(true);

  Serial.println("Initialization complete.");
}

// -------------------------------------------------------
// MAIN LOOP
// -------------------------------------------------------
void loop() {
  // Get the currently touched pads
  currtouched = cap.touched();

  for (uint8_t i = 0; i < 12; i++) {
    // If pad *is* touched and *wasn't* touched before → call onTouched
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i))) {
      onTouched(i);
    }
    // If pad *was* touched and now *isn't* → call onReleased
    if (!(currtouched & _BV(i)) && (lasttouched & _BV(i))) {
      onReleased(i);
    }
  }

  // Reset our state
  lasttouched = currtouched;

  // comment out this line for detailed data from the sensor!
  return;

  // Debugging info below (unreachable unless you remove the return above)
  Serial.print("\t\t\t\t\t\t\t\t\t\t\t\t\t 0x"); Serial.println(cap.touched(), HEX);
  Serial.print("Filt: ");
  for (uint8_t i = 0; i < 12; i++) {
    Serial.print(cap.filteredData(i)); Serial.print("\t");
  }
  Serial.println();
  Serial.print("Base: ");
  for (uint8_t i = 0; i < 12; i++) {
    Serial.print(cap.baselineData(i)); Serial.print("\t");
  }
  Serial.println();

  delay(100);
}