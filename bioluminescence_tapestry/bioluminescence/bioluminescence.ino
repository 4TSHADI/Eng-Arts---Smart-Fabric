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
#include "SolidColorMode.h"

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

static const uint8_t PANEL_CHANNELS[NUM_PANELS] = {0, 1};
static const uint8_t X_PIN_MIN = 0;
static const uint8_t X_PIN_MAX = 5;
static const uint8_t Y_PIN_MIN = 6;
static const uint8_t Y_PIN_MAX = 11;

struct AxisTouch {
  bool valid;
  uint8_t pin;
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
  Serial.println("Initialising MPR121 sensors via TCA9548A...");

  for (uint8_t p = 0; p < NUM_PANELS; p++) {
    uint8_t ch = PANEL_CHANNELS[p];
    tcaSelect(ch);
    if (caps[ch].begin(MPR121_ADDR)) {
      caps[ch].setAutoconfig(true);
      caps[ch].setThresholds(12, 6);
      Serial.print("  MPR121 panel ");
      Serial.print(p);
      Serial.print(" (Mux Ch ");
      Serial.print(ch);
      Serial.println(") found.");
      mprReady[ch] = true;
    } else {
      Serial.print("  MPR121 panel ");
      Serial.print(p);
      Serial.print(" (Mux Ch ");
      Serial.print(ch);
      Serial.println(") NOT found.");
      mprReady[ch] = false;
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

// ─────────────────────────────────────────────────────────────
// loop()
// ─────────────────────────────────────────────────────────────
void tcaSelect(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(TCA9548A_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

inline uint8_t xPinToCell(uint8_t pin) {
  // X labels are right-to-left: 0,1,2,3,4,5.
  return (uint8_t)(5 - (pin - X_PIN_MIN));
}

inline uint8_t yPinToCell(uint8_t pin) {
  // Y labels are top-to-bottom: 6,7,8,9,10,11.
  return (uint8_t)(pin - Y_PIN_MIN);
}

AxisTouch getStrongestAxisTouch(uint8_t channel, uint8_t pinMin, uint8_t pinMax) {
  AxisTouch result = {false, 0, 0};

  if (!mprReady[channel]) return result;

  tcaSelect(channel);
  uint16_t touched = caps[channel].touched();

  int16_t bestPressure = -1;
  for (uint8_t i = pinMin; i <= pinMax; i++) {
    if (!(touched & _BV(i))) continue;

    int16_t pressure = (int16_t)caps[channel].baselineData(i) - (int16_t)caps[channel].filteredData(i);
    if (pressure < 0) pressure = 0;

    if (!result.valid || pressure > bestPressure) {
      result.valid = true;
      result.pin = i;
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
    uint8_t ch = PANEL_CHANNELS[panel];
    if (!mprReady[ch]) continue;

    AxisTouch xTouch = getStrongestAxisTouch(ch, X_PIN_MIN, X_PIN_MAX);
    AxisTouch yTouch = getStrongestAxisTouch(ch, Y_PIN_MIN, Y_PIN_MAX);

    bool currentTouch = xTouch.valid && yTouch.valid;

    if (currentTouch) {
      uint8_t xCell = xPinToCell(xTouch.pin);
      uint8_t yCell = yPinToCell(yTouch.pin);
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
