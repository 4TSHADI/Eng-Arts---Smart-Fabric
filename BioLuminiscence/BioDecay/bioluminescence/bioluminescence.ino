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

// MPR121 #1 – ADDR → GND  → 0x5A  (global pins  0-11)
Adafruit_MPR121   cap1;
bool              mpr1Ready = false;
uint16_t          lasttouched1 = 0;
uint16_t          currtouched1 = 0;

// MPR121 #2 – ADDR → 3.3V → 0x5B  (global pins 12-23)
Adafruit_MPR121   cap2;
bool              mpr2Ready = false;
uint16_t          lasttouched2 = 0;
uint16_t          currtouched2 = 0;

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

  // ── MPR121 #1 (0x5A, ADDR → GND) ────────────────────────
  Wire.begin();
  Serial.println("Initialising MPR121 #1 (0x5A)...");
  if (cap1.begin(MPR121_ADDR1)) {
    cap1.setAutoconfig(true);
    cap1.setThresholds(12, 6);
    Serial.println("  MPR121 #1 found.");
    mpr1Ready = true;
  } else {
    Serial.println("  MPR121 #1 NOT found. Check ADDR→GND and I2C wiring.");
  }

  // ── MPR121 #2 (0x5B, ADDR → 3.3V) ───────────────────────
  Serial.println("Initialising MPR121 #2 (0x5B)...");
  if (cap2.begin(MPR121_ADDR2)) {
    cap2.setAutoconfig(true);
    cap2.setThresholds(12, 6);
    Serial.println("  MPR121 #2 found.");
    mpr2Ready = true;
  } else {
    Serial.println("  MPR121 #2 NOT found. Check ADDR→3.3V and I2C wiring.");
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
void loop() {
  // 1. Serial mode switching
  checkSerialInput();

  // 2a. MPR121 #1 → global pins 0-11
  if (mpr1Ready) {
    currtouched1 = cap1.touched();

    for (uint8_t i = 0; i < 12; i++) {
      int16_t pressure = (int16_t)cap1.baselineData(i) - (int16_t)cap1.filteredData(i);
      if (pressure < 0) pressure = 0;

      if ((currtouched1 & _BV(i)) && !(lasttouched1 & _BV(i))) {
        Serial.print("[MPR1] electrode "); Serial.print(i); Serial.println(" touched");
        activeMode->onTouch(strip, i, true, pressure);   // global pin = i
      }
      if (!(currtouched1 & _BV(i)) && (lasttouched1 & _BV(i))) {
        Serial.print("[MPR1] electrode "); Serial.print(i); Serial.println(" released");
        activeMode->onTouch(strip, i, false, pressure);
      }
    }
    lasttouched1 = currtouched1;
  }

  // 2b. MPR121 #2 → global pins 12-23
  if (mpr2Ready) {
    currtouched2 = cap2.touched();

    for (uint8_t i = 0; i < 12; i++) {
      int16_t pressure = (int16_t)cap2.baselineData(i) - (int16_t)cap2.filteredData(i);
      if (pressure < 0) pressure = 0;

      uint8_t globalPin = i + 12;  // offset into second sensor's region range

      if ((currtouched2 & _BV(i)) && !(lasttouched2 & _BV(i))) {
        Serial.print("[MPR2] electrode "); Serial.print(i);
        Serial.print(" touched (global pin "); Serial.print(globalPin); Serial.println(")");
        activeMode->onTouch(strip, globalPin, true, pressure);
      }
      if (!(currtouched2 & _BV(i)) && (lasttouched2 & _BV(i))) {
        Serial.print("[MPR2] electrode "); Serial.print(i);
        Serial.print(" released (global pin "); Serial.print(globalPin); Serial.println(")");
        activeMode->onTouch(strip, globalPin, false, pressure);
      }
    }
    lasttouched2 = currtouched2;
  }

  // 3. Continuous animation update for the active mode
  activeMode->update(strip);

  delay(10); // ~100 Hz scan rate
}
