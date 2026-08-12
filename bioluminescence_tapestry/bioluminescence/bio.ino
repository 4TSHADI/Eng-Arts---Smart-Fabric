// ============================================================
// BioDecay.ino
// Main sketch – Bioluminescent Decay System
// Modified: each 16x16 panel now uses 8x8 electrodes,
// split across two dedicated MPR121 chips (col chip + row chip),
// each using the non-contiguous pins {0,1,2,3,4,5,6,11}.
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
#include "SolidColorMode.h"

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

// ── Hardware objects ──────────────────────────────────────────
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_BGR + NEO_KHZ800);

// ── Electrode config: 8 electrodes per axis, non-contiguous pins ──
#define NUM_ELECTRODES_PER_AXIS 8
static const uint8_t ELECTRODE_PINS[NUM_ELECTRODES_PER_AXIS] = {0, 1, 2, 3, 4, 5, 6, 11};

// ── Now 4 MPR121 chips total: each panel gets a dedicated col chip + row chip ──
#define NUM_MPR_CHIPS (NUM_PANELS * 2)
Adafruit_MPR121 caps[NUM_MPR_CHIPS];
bool             mprReady[NUM_MPR_CHIPS];

// Mux channel assignment: panel p -> colChip = p*2, rowChip = p*2 + 1
inline uint8_t colChip(uint8_t panel) { return panel * 2; }
inline uint8_t rowChip(uint8_t panel) { return panel * 2 + 1; }

struct AxisTouch {
  bool valid;
  uint8_t electrodeIndex; // 0-7, index into ELECTRODE_PINS
  int16_t pressure;
};

// ── Mode instances ────────────────────────────────────────────
SimpleDecayMode   simpleMode;
PulsingDecayMode  pulsingMode;
MultiExpDecayMode multiMode;
SpreadDecayMode   spreadMode;
HeatMapMode       heatMapMode;
SolidColorMode    solidColorMode;

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
void tcaSelect(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(TCA9548A_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

// ─────────────────────────────────────────────────────────────
// setup()
// ─────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  // while (!Serial) { delay(10); } // commented out to prevent hanging on some boards

  // ── LED matrix ────────────────────────────────────────────
  strip.begin();
  strip.show();
  Serial.println("=== BioDecay – NeoPixel ready ===");

  Wire.begin();
  Serial.println("Initialising MPR121 sensors (2 per panel, 8x8 electrodes) via TCA9548A...");

  for (uint8_t p = 0; p < NUM_PANELS; p++) {
    uint8_t cc = colChip(p);
    uint8_t rc = rowChip(p);

    tcaSelect(cc);
    if (caps[cc].begin(MPR121_ADDR)) {
      caps[cc].setAutoconfig(true);
      caps[cc].setThresholds(12, 6);
      Serial.print("  Panel "); Serial.print(p);
      Serial.print(" col chip (Mux Ch "); Serial.print(cc);
      Serial.println(") found.");
      mprReady[cc] = true;
    } else {
      Serial.print("  Panel "); Serial.print(p);
      Serial.print(" col chip (Mux Ch "); Serial.print(cc);
      Serial.println(") NOT found.");
      mprReady[cc] = false;
    }

    tcaSelect(rc);
    if (caps[rc].begin(MPR121_ADDR)) {
      caps[rc].setAutoconfig(true);
      caps[rc].setThresholds(12, 6);
      Serial.print("  Panel "); Serial.print(p);
      Serial.print(" row chip (Mux Ch "); Serial.print(rc);
      Serial.println(") found.");
      mprReady[rc] = true;
    } else {
      Serial.print("  Panel "); Serial.print(p);
      Serial.print(" row chip (Mux Ch "); Serial.print(rc);
      Serial.println(") NOT found.");
      mprReady[rc] = false;
    }
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
  Serial.println("  6 -> Solid Color    verify touch sections");
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
    case '6': switchMode(&solidColorMode); break;
    default:
      Serial.print("Unknown command: "); Serial.println(input);
      break;
  }
}

// X labels run right-to-left across the 8 electrodes; Y labels run top-to-bottom.
// Adjust these two lines if your panel's physical orientation differs.
inline uint8_t xIndexToCell(uint8_t idx) { return (uint8_t)(NUM_ELECTRODES_PER_AXIS - 1 - idx); }
inline uint8_t yIndexToCell(uint8_t idx) { return idx; }

// Reads one axis (col chip or row chip) for a panel, returns strongest touched electrode
AxisTouch getStrongestAxisTouch(uint8_t chip) {
  AxisTouch result = {false, 0, 0};

  if (!mprReady[chip]) return result;

  tcaSelect(chip);
  uint16_t touched = caps[chip].touched();

  int16_t bestPressure = -1;
  for (uint8_t e = 0; e < NUM_ELECTRODES_PER_AXIS; e++) {
    uint8_t pin = ELECTRODE_PINS[e];
    if (!(touched & _BV(pin))) continue;

    int16_t pressure = (int16_t)caps[chip].baselineData(pin) - (int16_t)caps[chip].filteredData(pin);
    if (pressure < 0) pressure = 0;

    if (!result.valid || pressure > bestPressure) {
      result.valid = true;
      result.electrodeIndex = e;
      result.pressure = pressure;
      bestPressure = pressure;
    }
  }

  return result;
}

void loop() {
  // 1. Serial mode switching
  checkSerialInput();

  static bool wasTouched[NUM_PANELS] = {false, false};
  static uint8_t lastXCell[NUM_PANELS] = {0, 0};
  static uint8_t lastYCell[NUM_PANELS] = {0, 0};

  for (uint8_t panel = 0; panel < NUM_PANELS; panel++) {
    uint8_t cc = colChip(panel);
    uint8_t rc = rowChip(panel);
    if (!mprReady[cc] || !mprReady[rc]) continue;

    AxisTouch xTouch = getStrongestAxisTouch(cc);
    AxisTouch yTouch = getStrongestAxisTouch(rc);

    bool currentTouch = xTouch.valid && yTouch.valid;

    if (currentTouch) {
      uint8_t xCell = xIndexToCell(xTouch.electrodeIndex);
      uint8_t yCell = yIndexToCell(yTouch.electrodeIndex);
      int16_t pressure = (xTouch.pressure + yTouch.pressure) / 2;

      if (!wasTouched[panel] || xCell != lastXCell[panel] || yCell != lastYCell[panel]) {
        if (wasTouched[panel]) {
          TouchEvent offEvent = {panel, lastXCell[panel], lastYCell[panel], false, 0};
          activeMode->onTouch(strip, offEvent);
        }

        TouchEvent onEvent = {panel, xCell, yCell, true, pressure};
        activeMode->onTouch(strip, onEvent);

        lastXCell[panel] = xCell;
        lastYCell[panel] = yCell;

        Serial.print("LED"); Serial.print(panel + 1);
        Serial.print(" touch -> xCell "); Serial.print(xCell);
        Serial.print("  yCell "); Serial.print(yCell);
        Serial.print("  pressure "); Serial.println(pressure);
      }

      wasTouched[panel] = true;
    } else if (wasTouched[panel]) {
      TouchEvent offEvent = {panel, lastXCell[panel], lastYCell[panel], false, 0};
      activeMode->onTouch(strip, offEvent);
      wasTouched[panel] = false;
      Serial.print("LED"); Serial.print(panel + 1); Serial.println(" touch ended");
    }
  }

  // 3. Continuous animation update for the active mode
  activeMode->update(strip);

  delay(10);
}