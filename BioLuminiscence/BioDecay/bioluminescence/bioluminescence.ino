/*
  Touch-driven bioluminescence sketch

  - Uses 4 MPR121 boards through the TCA9548A mux
  - Forms two X/Y sensor pairs, one for each LED panel
  - Converts each pair into panel-aware 8x8 touch coordinates
  - Sends those coordinates to the active lighting mode
*/

#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include "ProjectConfig.h"
#include "SensorHub.h"
#include "ModeController.h"
#include "WiFiController.h"
#include "SimpleDecayMode.h"
#include "PulsingDecayMode.h"
#include "MultiExpDecayMode.h"
#include "SpreadDecayMode.h"
#include "HeatMapMode.h"
#include "SolidColorMode.h"

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
SensorHub sensorHub;
ModeController modeController;
WiFiController wifiController;

SimpleDecayMode simpleMode;
PulsingDecayMode pulsingMode;
MultiExpDecayMode multiExpMode;
SpreadDecayMode spreadMode;
HeatMapMode heatMapMode;
SolidColorMode solidColorMode;

void setup() {
  Serial.begin(9600);
  while (!Serial) { delay(10); }
  sensorHub.begin();

  strip.begin();
  strip.clear();
  strip.show();

  Serial.println("Bioluminescence 4-MPR dispatcher");
  Serial.println("Two X/Y MPR pairs mapped onto two 16x16 panels");

  modeController.registerMode("Simple Decay", &simpleMode);
  modeController.registerMode("Pulsing Decay", &pulsingMode);
  modeController.registerMode("Multi-Exp Decay", &multiExpMode);
  modeController.registerMode("Spread Diffusion", &spreadMode);
  modeController.registerMode("Heat Map", &heatMapMode);
  modeController.registerMode("Solid Color", &solidColorMode);

  modeController.printMenu();
  sensorHub.initSensors();
  modeController.begin(strip);
  wifiController.begin();
}

void loop() {
  modeController.handleSerial(strip);
  wifiController.handle(modeController, strip);

  DecayMode* activeMode = modeController.activeMode();
  sensorHub.pollTouches(activeMode, strip);

  if (activeMode != nullptr) {
    activeMode->update(strip);
  }

  delay(50);
}
