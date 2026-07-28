#include <Wire.h>
#include "Adafruit_MPR121.h"
#include <Adafruit_NeoPixel.h>

#include "ProjectConfig.h"
#include "bioluminescent.h"
#include "SolidColorMode.h"
#include "spread.h"
#include "LowBrightnessMode.h"

#define TCA9548A_ADDR 0x70
#define MPR121_ADDR 0x5A

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif
#define LED_PIN     6

// Setup NeoPixel Strip/Matrix
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_BGR + NEO_KHZ800);

// Hardware sensors
Adafruit_MPR121 cap[NUM_MUX_CHANNELS];
bool mprReady[NUM_MUX_CHANNELS];

uint16_t lasttouched[NUM_MUX_CHANNELS];
uint16_t currtouched[NUM_MUX_CHANNELS];

// Modes instantiations
SolidColorMode solidMode;
FlashingMode flashingMode;
LowBrightnessMode lowMode;

// Active mode pointer
LightingMode* activeMode = &solidMode;

void tcaSelect(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(TCA9548A_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

void switchMode(LightingMode* newMode, const char* name) {
  Serial.print("\n>>> Switching to mode: ");
  Serial.println(name);
  activeMode = newMode;
  activeMode->enter(strip);
}

void setup() {
  Serial.begin(9600);
  while (!Serial) { delay(10); }

  // Initialize NeoPixel Matrix
  strip.begin();
  strip.show(); // Clear strip
  Serial.println("=== NeoPixel Matrix Ready ===");

  Wire.begin();

  Serial.println("Initializing 6x MPR121 via TCA9548A...");

  for (uint8_t ch = 0; ch < NUM_MUX_CHANNELS; ch++) {
    tcaSelect(ch);
    delay(10); // Small delay to let the multiplexer switch
    if (cap[ch].begin(MPR121_ADDR)) {
      cap[ch].setAutoconfig(true);
      cap[ch].setThresholds(12, 6); // Set standard touch/release thresholds

      Serial.print("Channel "); Serial.print(ch); Serial.println(": MPR121 found.");
      mprReady[ch] = true;
    } else {
      Serial.print("Channel "); Serial.print(ch); Serial.println(": MPR121 NOT found.");
      mprReady[ch] = false;
    }
    lasttouched[ch] = 0;
  }

  // Initialize active mode
  activeMode->enter(strip);

  Serial.println("\n=== Setup complete. ===");
  Serial.println("Type mode number in Serial Monitor to switch:");
  Serial.println("  1 -> Solid Region Colors");
  Serial.println("  2 -> Flashing Response");
  Serial.println("  3 -> Low Brightness Solid Colors");
}

void checkSerialInput() {
  if (Serial.available() > 0) {
    char input = Serial.read();
    
    // Ignore newlines/carriage returns
    if (input == '\r' || input == '\n') return;

    switch (input) {
      case '1':
        switchMode(&solidMode, "Solid Region Colors (Normal)");
        break;
      case '2':
        switchMode(&flashingMode, "Flashing Response");
        break;
      case '3':
        switchMode(&lowMode, "Low Brightness Solid Colors");
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

  for (uint8_t ch = 0; ch < NUM_MUX_CHANNELS; ch++) {
    if (!mprReady[ch]) continue;

    tcaSelect(ch);
    currtouched[ch] = cap[ch].touched();

    for (uint8_t i = 0; i < 12; i++) {
      // Electrode Touched
      if ((currtouched[ch] & _BV(i)) && !(lasttouched[ch] & _BV(i))) {
        Serial.print("Ch "); Serial.print(ch);
        Serial.print(" - electrode "); Serial.print(i);
        Serial.println(" touched");
        
        activeMode->onTouch(strip, ch, i, true);
      }
      // Electrode Released
      if (!(currtouched[ch] & _BV(i)) && (lasttouched[ch] & _BV(i))) {
        Serial.print("Ch "); Serial.print(ch);
        Serial.print(" - electrode "); Serial.print(i);
        Serial.println(" released");
        
        activeMode->onTouch(strip, ch, i, false);
      }
    }

    lasttouched[ch] = currtouched[ch];
  }

  // Perform continuous updates for active mode (e.g. flashing animations)
  activeMode->update(strip);

  delay(10); // Fast touch scanning
}
