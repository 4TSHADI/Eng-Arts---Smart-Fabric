#include <Wire.h>
#include "Adafruit_MPR121.h"
#include <Adafruit_NeoPixel.h>

#include "ProjectConfig.h"
#include "solidColorMode.h"
#include "bioluminescentMode.h"
#include "spreadMode.h"

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

// Setup NeoPixel Strip/Matrix with NEO_BGR color order as configured in your testing scripts
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_BGR + NEO_KHZ800);

// Hardware sensor (Single MPR121 directly on I2C bus)
Adafruit_MPR121 cap;
bool mprReady = false;

uint16_t lasttouched = 0;
uint16_t currtouched = 0;

// Modes instantiations
SolidColorMode solidMode;
BioluminescentMode bioMode;
SpreadMode spreadMode;

// Active mode pointer
LightingMode* activeMode = &solidMode;

void switchMode(LightingMode* newMode) {
  Serial.print("\n>>> Switching to mode: ");
  Serial.println(newMode->getName());
  activeMode = newMode;
  activeMode->enter(strip);
}

void setup() {
  Serial.begin(9600);
  while (!Serial) { delay(10); }

  // Initialize NeoPixel Matrix
  strip.begin();
  strip.show(); // Clear strip (all off)
  Serial.println("=== NeoPixel Matrix Ready ===");

  Wire.begin();

  Serial.println("Initializing single MPR121...");
  if (cap.begin(MPR121_ADDR)) {
    cap.setAutoconfig(true);
    cap.setThresholds(12, 6); // Set standard touch/release thresholds
    Serial.println("MPR121 found.");
    mprReady = true;
  } else {
    Serial.println("MPR121 NOT found. Please check I2C wiring and pullups!");
    mprReady = false;
  }

  // Initialize active mode
  activeMode->enter(strip);

  Serial.println("\n=== Setup complete. ===");
  Serial.println("Type mode number in Serial Monitor to switch:");
  Serial.println("  1 -> Solid Red Mode");
  Serial.println("  2 -> Bioluminescent  Mode");
  Serial.println("  3 -> Spread Mode");
}

void checkSerialInput() {
  if (Serial.available() > 0) {
    char input = Serial.read();
    
    // Ignore newlines/carriage returns
    if (input == '\r' || input == '\n') return;

    switch (input) {
      case '1':
        switchMode(&solidMode);
        break;
      case '2':
        switchMode(&bioMode);
        break;
      case '3':
        switchMode(&spreadMode);
        break;
      default:
        Serial.print("Unknown mode command: ");
        Serial.println(input);
        break;
    }
  }
}

void loop() {
  // Check if user requested a mode switch via Serial
  checkSerialInput();

  if (mprReady) {
    currtouched = cap.touched();

    for (uint8_t i = 0; i < 12; i++) {
      int16_t pressure = 0;
      if (mprReady) {
        // Read baseline and filtered data to calculate touch pressure
        pressure = (int16_t)cap.baselineData(i) - (int16_t)cap.filteredData(i);
        if (pressure < 0) pressure = 0; // Guard against minor noise fluctuations
      }

      // Electrode Touched
      if ((currtouched & _BV(i)) && !(lasttouched & _BV(i))) {
        // Serial.print("Electrode "); Serial.print(i); Serial.println(" touched");
        activeMode->onTouch(strip, i, true, pressure);
      }
      // Electrode Released
      if (!(currtouched & _BV(i)) && (lasttouched & _BV(i))) {
        // Serial.print("Electrode "); Serial.print(i); Serial.println(" released");
        activeMode->onTouch(strip, i, false);
      }
    }

    lasttouched = currtouched;
  }

  // Perform continuous updates for active mode
  activeMode->update(strip);

  delay(10); // Fast touch scanning
}
