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
#include <WiFiS3.h>

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

struct ModeBinding {
  char command;
  DecayMode* mode;
  const char* help;
};

// ── Hardware objects ──────────────────────────────────────────
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_BGR + NEO_KHZ800);
WiFiServer webServer(WIFI_HTTP_PORT);

// One MPR121 per mux channel (6 total sensors)
Adafruit_MPR121   caps[NUM_MPR_SENSORS];
bool              mprReady[NUM_MPR_SENSORS];
uint16_t          lasttouched[NUM_MPR_SENSORS];
uint16_t          currtouched[NUM_MPR_SENSORS];

// ── Mode instances ────────────────────────────────────────────
SimpleDecayMode   simpleMode;
PulsingDecayMode  pulsingMode;
MultiExpDecayMode multiMode;
SpreadDecayMode   spreadMode;
HeatMapMode       heatMapMode;
SolidColorMode    solidColorMode;

// ── Active mode pointer (mirrors mpr121_multi_mode pattern) ──
DecayMode* activeMode = &simpleMode;

ModeBinding modeBindings[] = {
  { '1', &simpleMode,  "Simple Decay   I(t)=I0*exp(-t/tau)" },
  { '2', &pulsingMode, "Pulsing Decay  I(t)=I0*exp(-t/tau)*(1+a*sin(wt))" },
  { '3', &multiMode,   "Multi-Exp      I(t)=A1*exp(-t/t1)+A2*exp(-t/t2)" },
  { '4', &spreadMode,  "Spread Diffusion  particle-based bloom" },
  { '5', &heatMapMode, "Heat Map       touch memory: red=hot, green=cool" },
  { '6', &solidColorMode, "Solid Color    pin-to-section calibration" },
};

const uint8_t MODE_COUNT = sizeof(modeBindings) / sizeof(modeBindings[0]);

void tcaSelect(uint8_t channel);
void initSensors();
void pollTouchEvents();
void printModeHelp();
DecayMode* findModeByCommand(char command);
void initWiFiControl();
void pollWiFiControl();
void serveControlPage(WiFiClient& client);
void processModeCommand(char input, const char* sourceTag);

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
  initWiFiControl();
  initSensors();

  // ── Start in Mode 1 ───────────────────────────────────────
  activeMode->enter(strip);

  Serial.println("\n=== Setup complete ===");
  Serial.println("WiFi mode switch enabled. Open / on device IP and tap a mode.");
  printModeHelp();
}

// ─────────────────────────────────────────────────────────────
// checkSerialInput() – mirror of Eng-Arts version
// ─────────────────────────────────────────────────────────────
void checkSerialInput() {
  while (Serial.available() > 0) {
    processModeCommand((char)Serial.read(), "USB");
  }
}

void initWiFiControl() {
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("[WiFi] No WiFi module detected.");
    return;
  }

  Serial.print("[WiFi] Connecting to SSID: ");
  Serial.println(WIFI_SSID);

  while (WiFi.begin(WIFI_SSID, WIFI_PASS) != WL_CONNECTED) {
    Serial.println("[WiFi] Connection failed, retrying in 3s...");
    delay(3000);
  }

  webServer.begin();
  Serial.print("[WiFi] Connected. IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("[WiFi] Control URL: http://");
  Serial.println(WiFi.localIP());
}

void processModeCommand(char input, const char* sourceTag) {
  if (input == '\r' || input == '\n' || input == ' ' || input == '\t') return;

  DecayMode* mode = findModeByCommand(input);
  if (mode) {
    Serial.print("["); Serial.print(sourceTag); Serial.print("] command: ");
    Serial.println(input);
    switchMode(mode);
    return;
  }

  // Only show help when explicitly requested; ignore all other stray bytes.
  if (input == '?' || input == 'h' || input == 'H') {
    printModeHelp();
  }
}

void serveControlPage(WiFiClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  client.println("<title>BioDecay Mode Control</title>");
  client.println("<style>body{font-family:Arial,sans-serif;background:#04151f;color:#d9f3ff;margin:0;padding:20px}h1{font-size:22px}a{display:block;margin:10px 0;padding:14px;border-radius:10px;text-decoration:none;background:#0f4c75;color:white;text-align:center;font-size:18px}small{opacity:.8}</style>");
  client.println("</head><body><h1>BioDecay Mode Control</h1>");
  client.println("<a href='/mode/1'>Mode 1 - Simple Decay</a>");
  client.println("<a href='/mode/2'>Mode 2 - Pulsing Decay</a>");
  client.println("<a href='/mode/3'>Mode 3 - Multi-Exp Decay</a>");
  client.println("<a href='/mode/4'>Mode 4 - Spread Diffusion</a>");
  client.println("<a href='/mode/5'>Mode 5 - Heat Map</a>");
  client.println("<a href='/mode/6'>Mode 6 - Solid Color Calibration</a>");
  client.println("<small>Tip: bookmark this page on your phone.</small>");
  client.println("</body></html>");
}

void pollWiFiControl() {
  WiFiClient client = webServer.available();
  if (!client) return;

  String requestLine;
  unsigned long start = millis();

  while (client.connected() && millis() - start < 500) {
    if (!client.available()) continue;

    requestLine = client.readStringUntil('\n');
    requestLine.trim();

    if (requestLine.startsWith("GET /mode/") && requestLine.length() >= 11) {
      char modeChar = requestLine.charAt(10);
      processModeCommand(modeChar, "WiFi");
    }

    while (client.available()) {
      String header = client.readStringUntil('\n');
      if (header == "\r" || header.length() == 0) break;
    }

    serveControlPage(client);
    break;
  }

  delay(1);
  client.stop();
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

void initSensors() {
  Serial.println("Initialising MPR121 sensors via TCA9548A...");

  for (uint8_t sensorIdx = 0; sensorIdx < NUM_MPR_SENSORS; sensorIdx++) {
    const SensorBinding& binding = SENSOR_BINDINGS[sensorIdx];
    tcaSelect(binding.muxChannel);

    if (caps[sensorIdx].begin(binding.i2cAddr)) {
      caps[sensorIdx].setAutoconfig(true);
      caps[sensorIdx].setThresholds(12, 6);
      Serial.print("  Sensor "); Serial.print(sensorIdx);
      Serial.print(" -> mux "); Serial.print(binding.muxChannel);
      Serial.print(" panel "); Serial.print(binding.panelId);
      Serial.print(" axis "); Serial.print(binding.axisId);
      Serial.print(" (0x"); Serial.print(binding.i2cAddr, HEX); Serial.println(") found.");
      mprReady[sensorIdx] = true;
    } else {
      Serial.print("  Sensor "); Serial.print(sensorIdx);
      Serial.print(" -> mux "); Serial.print(binding.muxChannel);
      Serial.print(" panel "); Serial.print(binding.panelId);
      Serial.print(" axis "); Serial.print(binding.axisId);
      Serial.print(" (0x"); Serial.print(binding.i2cAddr, HEX); Serial.println(") NOT found.");
      mprReady[sensorIdx] = false;
    }

    lasttouched[sensorIdx] = 0;
    currtouched[sensorIdx] = 0;
  }
}

void pollTouchEvents() {
  for (uint8_t sensorIdx = 0; sensorIdx < NUM_MPR_SENSORS; sensorIdx++) {
    if (!mprReady[sensorIdx]) continue;

    const SensorBinding& binding = SENSOR_BINDINGS[sensorIdx];
    tcaSelect(binding.muxChannel);
    currtouched[sensorIdx] = caps[sensorIdx].touched();

    for (uint8_t i = 0; i < 12; i++) {
      int16_t pressure = (int16_t)caps[sensorIdx].baselineData(i) -
                         (int16_t)caps[sensorIdx].filteredData(i);
        if (pressure < 0) pressure = 0;

      uint8_t globalPin = (uint8_t)(sensorIdx * 12 + i);
      bool touchedNow = (currtouched[sensorIdx] & _BV(i));
      bool touchedPrev = (lasttouched[sensorIdx] & _BV(i));

      if (touchedNow == touchedPrev) continue;

      TouchEvent event;
      event.panelId = binding.panelId;
      event.axisId = binding.axisId;
      event.electrode = i;
      event.globalPin = globalPin;
      event.isTouched = touchedNow;
      event.pressure = pressure;

      Serial.print("[MPR sensor="); Serial.print(sensorIdx);
      Serial.print(" mux="); Serial.print(binding.muxChannel);
      Serial.print(" panel="); Serial.print(binding.panelId);
      Serial.print(" axis="); Serial.print(binding.axisId);
      Serial.print("] electrode "); Serial.print(i);
      Serial.print(touchedNow ? " touched" : " released");
      Serial.print(" (global pin "); Serial.print(globalPin); Serial.println(")");

      activeMode->onTouch(strip, event);
    }

    lasttouched[sensorIdx] = currtouched[sensorIdx];
  }
}

void printModeHelp() {
  Serial.println("Serial commands:");
  for (uint8_t i = 0; i < MODE_COUNT; i++) {
    Serial.print("  "); Serial.print(modeBindings[i].command);
    Serial.print(" -> "); Serial.println(modeBindings[i].help);
  }
}

DecayMode* findModeByCommand(char command) {
  for (uint8_t i = 0; i < MODE_COUNT; i++) {
    if (modeBindings[i].command == command) return modeBindings[i].mode;
  }
  return nullptr;
}

void loop() {
  // 1. Serial mode switching
  checkSerialInput();
  // 1b. WiFi mode switching
  pollWiFiControl();
  // 2. Poll fabric touch sensors and dispatch normalized events
  pollTouchEvents();

  // 3. Continuous animation update for the active mode
  activeMode->update(strip);

  delay(10);
}
