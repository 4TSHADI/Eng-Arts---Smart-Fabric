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
#include "SimpleLightingMode.h"
#include "PulsingLightingMode.h"
#include "MultiExpLightingMode.h"
#include "SpreadLightingMode.h"
#include "HeatMapMode.h"
#include "SolidColorMode.h"
#include "FlashTestMode.h"

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
SensorHub sensorHub;
ModeController modeController;
WiFiController wifiController;

SimpleLightingMode simpleMode;
PulsingLightingMode pulsingMode;
MultiExpLightingMode multiExpMode;
SpreadLightingMode spreadMode;
HeatMapMode heatMapMode;
SolidColorMode solidColorMode;
FlashTestMode flashTestMode;

// Non-blocking scheduler intervals.
static const uint16_t SENSOR_POLL_INTERVAL_MS = 5;   // ~200 Hz touch polling target
static const uint16_t RENDER_INTERVAL_MS      = 16;  // ~60 FPS animation target
static unsigned long lastSensorPollMs = 0;
static unsigned long lastRenderMs = 0;

void setup() {
  Serial.begin(9600);
  unsigned long serialWaitStart = millis();
  while (!Serial && (millis() - serialWaitStart) < SERIAL_WAIT_TIMEOUT_MS) {
    delay(10);
  }
  sensorHub.begin();

  strip.begin();
  strip.clear();
  strip.show();

  Serial.println("Bioluminescence 4-MPR dispatcher");
  Serial.println("Two X/Y MPR pairs mapped onto two 16x16 panels");

  modeController.registerMode("Simple Lighting", &simpleMode);
  modeController.registerMode("Pulsing Lighting", &pulsingMode);
  modeController.registerMode("Multi-Exp Lighting", &multiExpMode);
  modeController.registerMode("Spread Diffusion", &spreadMode);
  modeController.registerMode("Heat Map", &heatMapMode);
  modeController.registerMode("Solid Color", &solidColorMode);
  modeController.registerMode("Flash Test", &flashTestMode);
                                                             
  modeController.printMenu();
  sensorHub.initSensors();
  modeController.begin(strip);
  if (ENABLE_WIFI_CONTROL) {
    wifiController.begin();
  } else {
    Serial.println("WiFi control disabled (ENABLE_WIFI_CONTROL=0)");
  }
}

void loop() {
  unsigned long now = millis();

  modeController.handleSerial(strip);
  wifiController.handle(modeController, strip);

  LightingMode* activeMode = modeController.activeMode();

  if (activeMode != nullptr && (now - lastSensorPollMs) >= SENSOR_POLL_INTERVAL_MS) {
    lastSensorPollMs = now;
    sensorHub.pollTouches(activeMode, strip);
  }

  if (activeMode != nullptr && (now - lastRenderMs) >= RENDER_INTERVAL_MS) {
    lastRenderMs = now;
    activeMode->update(strip);
  }
}
