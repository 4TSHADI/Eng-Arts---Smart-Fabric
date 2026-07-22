// ============================================================
// BioDecay.ino
// Main sketch – Bioluminescent Decay System
//
// Mode switching follows the exact pattern from mpr121_multi_mode:
//   • LightingMode  →  DecayMode  (abstract base)
//   • switchMode()  →  identical signature
//   • checkSerialInput() → type '1', '2', '3' in Serial Monitor
//   • loop() polls MPR121 → fires onTouch() → calls update()
//
// ┌────────────────────────────────────────────────────────────┐
// │  '1'  SimpleDecayMode   – I(t) = I₀·e^(−t/τ)             │
// │  '2'  PulsingDecayMode  – I(t) = I₀·e^(−t/τ)·(1+a·sinωt)│
// │  '3'  MultiExpDecayMode – I(t) = A₁e^(−t/τ₁)+A₂e^(−t/τ₂)│
// │  '4'  SpreadDecayMode   – particle bloom from touch points │
// │  '5'  HeatMapMode       – touch memory, red=hot green=cool │
// └────────────────────────────────────────────────────────────┘
// ============================================================

#include <Wire.h>
#include "Adafruit_MPR121.h"
#include <Adafruit_NeoPixel.h>

#include "ProjectConfig.h"
#include "DecayMode.h"
#include "SimpleDecayMode.h"
#include "PulsingDecayMode.h"
#include "MultiExpDecayMode.h"
#include "SpreadDecayMode.h"
#include "HeatMapMode.h"

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

// ── Hardware objects ──────────────────────────────────────────
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_BGR + NEO_KHZ800);

// MPR121 array for mux-based tapestry expansion
Adafruit_MPR121   caps[NUM_MUX_CHANNELS];
bool              mprReady[NUM_MUX_CHANNELS];
uint16_t          lasttouched[NUM_MUX_CHANNELS];
uint16_t          currtouched[NUM_MUX_CHANNELS];

// ── Mode instances ────────────────────────────────────────────
SimpleDecayMode   simpleMode;
PulsingDecayMode  pulsingMode;
MultiExpDecayMode multiMode;
SpreadDecayMode   spreadMode;
HeatMapMode       heatMapMode;

// ── Active mode pointer (mirrors mpr121_multi_mode pattern) ──
DecayMode* activeMode = &simpleMode;

// ─────────────────────────────────────────────────────────────
// switchMode() – identical pattern to the Eng-Arts codebase
// ─────────────────────────────────────────────────────────────
void switchMode(DecayMode* newMode) {
  Serial.print("\n>>> Switching to mode: ");
  Serial.println(newMode->getName());
  activeMode = newMode;
  activeMode->enter(strip);
}

// ─────────────────────────────────────────────────────────────
// setup()
// ─────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  while (!Serial) { delay(10); }

  // ── LED matrix ────────────────────────────────────────────
  strip.begin();
  strip.show();
  Serial.println("=== BioDecay – NeoPixel ready ===");

  Wire.begin();
  Serial.println("Initialising MPR121 sensors via TCA9548A...");

  for (uint8_t ch = 0; ch < NUM_MUX_CHANNELS; ch++) {
    tcaSelect(ch);
    if (caps[ch].begin(MPR121_ADDR)) {
      caps[ch].setAutoconfig(true);
      caps[ch].setThresholds(12, 6);
      Serial.print("  MPR121 channel "); Serial.print(ch); Serial.println(" found.");
      mprReady[ch] = true;
    } else {
      Serial.print("  MPR121 channel "); Serial.print(ch); Serial.println(" NOT found.");
      mprReady[ch] = false;
    }
    lasttouched[ch] = 0;
  }

  // ── Start in Mode 1 ───────────────────────────────────────
  activeMode->enter(strip);

  Serial.println("\n=== Setup complete ===");
  Serial.println("Serial commands:");
  Serial.println("  1 -> Simple Decay   I(t) = I0*exp(-t/tau)");
  Serial.println("  2 -> Pulsing Decay  I(t) = I0*exp(-t/tau)*(1+a*sin(wt))");
  Serial.println("  3 -> Multi-Exp      I(t) = A1*exp(-t/t1) + A2*exp(-t/t2)");
  Serial.println("  4 -> Spread Diffusion  particle-based bloom from touch points");
  Serial.println("  5 -> Heat Map       touch memory: red=hot, green=cool");
}

// ─────────────────────────────────────────────────────────────
// checkSerialInput() – mirror of Eng-Arts version
// ─────────────────────────────────────────────────────────────
void checkSerialInput() {
  if (Serial.available() == 0) return;

  char input = Serial.read();
  if (input == '\r' || input == '\n') return; // ignore line endings

  switch (input) {
    case '1': switchMode(&simpleMode);  break;
    case '2': switchMode(&pulsingMode); break;
    case '3': switchMode(&multiMode);   break;
    case '4': switchMode(&spreadMode);  break;
    case '5': switchMode(&heatMapMode); break;
    default:
      Serial.print("Unknown command: "); Serial.println(input);
      break;
  }
}

// ─────────────────────────────────────────────────────────────
// loop()
// ─────────────────────────────────────────────────────────────
void tcaSelect(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(TCA9548A_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

void loop() {
  // 1. Serial mode switching
  checkSerialInput();

  for (uint8_t ch = 0; ch < NUM_MUX_CHANNELS; ch++) {
    if (!mprReady[ch]) continue;

    tcaSelect(ch);
    currtouched[ch] = caps[ch].touched();

    for (uint8_t i = 0; i < 12; i++) {
      int16_t pressure = (int16_t)caps[ch].baselineData(i) - (int16_t)caps[ch].filteredData(i);
      if (pressure < 0) pressure = 0;

      uint8_t globalPin = (ch * 12) + i;

      if ((currtouched[ch] & _BV(i)) && !(lasttouched[ch] & _BV(i))) {
        Serial.print("[MPR]"); Serial.print(ch);
        Serial.print(" electrode "); Serial.print(i);
        Serial.print(" touched (global pin "); Serial.print(globalPin); Serial.println(")");
        activeMode->onTouch(strip, globalPin, true, pressure);
      }
      if (!(currtouched[ch] & _BV(i)) && (lasttouched[ch] & _BV(i))) {
        Serial.print("[MPR]"); Serial.print(ch);
        Serial.print(" electrode "); Serial.print(i);
        Serial.print(" released (global pin "); Serial.print(globalPin); Serial.println(")");
        activeMode->onTouch(strip, globalPin, false, pressure);
      }
    }

    lasttouched[ch] = currtouched[ch];
  }

  // 3. Continuous animation update for the active mode
  activeMode->update(strip);

  delay(10);
}
