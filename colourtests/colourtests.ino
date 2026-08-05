#include <Wire.h>
#include <Adafruit_MPR121.h>
#include <Adafruit_NeoPixel.h>

// --- NeoPixel Setup ---
#define LED_PIN 6
#define LED_COUNT 256 // 16x16 matrix = 256 LEDs
// Note: Keep brightness low (e.g. 50) so the matrix doesn't draw too much power from the Arduino
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// --- MPR121 Setup ---
Adafruit_MPR121 cap = Adafruit_MPR121();
// Use electrodes that are physically connected on your sensor-0 layout.
const int BARE_PIN = 6;
const int YARN_PIN = 0;

// --- Mux Setup ---
static const uint8_t TCA9548A_ADDR = 0x70;
static const uint8_t MPR_MUX_CHANNEL = 0;

static const uint8_t MPR121_ADDR = 0x5A;
static const uint8_t REG_ECR = 0x5E;
static const uint8_t REG_CONFIG1 = 0x5C;
static const uint8_t REG_CONFIG2 = 0x5D;

// --- Software Detector Variables ---
float softBaseline[12]; 
bool softTouched[12];
const int TOUCH_THRESHOLDS[12]   = {8, 15, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12}; 
const int RELEASE_THRESHOLDS[12] = {4, 10,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8}; 

// Sensitivity knobs for your active electrodes.
const int SMALL_TOUCH_THRESHOLD = 4;
const int SMALL_RELEASE_THRESHOLD = 2;
const float BASELINE_UPDATE_ALPHA = 0.02f;

bool advancedMode = false; // We start in the "broken" default state for demonstration
unsigned long lastDebugPrint = 0;

void tcaSelect(uint8_t channel) {
  Wire.beginTransmission(TCA9548A_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

// Applies high power to the yarn
void applyYarnAFE() {
  tcaSelect(MPR_MUX_CHANNEL);
  Wire.beginTransmission(MPR121_ADDR); Wire.write(REG_ECR); Wire.write(0x00); Wire.endTransmission(); delay(2);
  Wire.beginTransmission(MPR121_ADDR); Wire.write(REG_CONFIG1); Wire.write(0x3F); Wire.endTransmission();
  Wire.beginTransmission(MPR121_ADDR); Wire.write(REG_CONFIG2); Wire.write(0x24); Wire.endTransmission();
  Wire.beginTransmission(MPR121_ADDR); Wire.write(REG_ECR); Wire.write(0x8C); Wire.endTransmission(); delay(2);
}

// Resets the MPR121 chip back to Adafruit's factory defaults
void resetToDefaults() {
  tcaSelect(MPR_MUX_CHANNEL);
  cap.begin(MPR121_ADDR);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  // Initialize Matrix
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  strip.setBrightness(180); // 40 is bright enough, prevents Arduino power crashes

  Serial.println("Running red LED startup self-test...");

  // Red-only hardware sanity test.
  strip.fill(strip.Color(255, 0, 0), 0, LED_COUNT); strip.show(); delay(500);
  strip.clear(); strip.show(); delay(1000);
  strip.fill(strip.Color(255, 0, 0), 0, LED_COUNT); strip.show(); delay(500);
  strip.clear(); strip.show();
  Serial.println("Red LED startup self-test done.");

  Serial.println("\n=== Interactive Matrix Demonstration ===");

  Wire.begin();
  tcaSelect(MPR_MUX_CHANNEL);
  
  if (!cap.begin(MPR121_ADDR)) {
    Serial.println("MPR121 not found! Check wiring.");
    while(1);
  }
  // Lower hardware thresholds so smaller raw changes can trigger in default mode.
  cap.setThresholds(4, 2);
  Serial.println("MPR121 found on mux channel 0 (addr 0x5A)");
  Serial.print("Using BARE_PIN="); Serial.print(BARE_PIN);
  Serial.print(" YARN_PIN="); Serial.println(YARN_PIN);
  
  Serial.println("\nStarting in DEFAULT MODE (No Configuration).");
  Serial.println("  -> The bare wire should light up BLUE.");
  Serial.println("  -> The yarn will likely fail to light up GREEN.");
  Serial.println("\nType '1' and press Enter to switch to ADVANCED MODE.");
}

void updateSoftwareDetector() {
  tcaSelect(MPR_MUX_CHANNEL);
  for (int i = 0; i < 12; i++) {
    int currentRaw = cap.filteredData(i);
    int difference = abs(currentRaw - (int)softBaseline[i]);
    bool isActiveElectrode = (i == BARE_PIN || i == YARN_PIN);
    int touchThreshold = isActiveElectrode ? SMALL_TOUCH_THRESHOLD : TOUCH_THRESHOLDS[i];
    int releaseThreshold = isActiveElectrode ? SMALL_RELEASE_THRESHOLD : RELEASE_THRESHOLDS[i];
    
    if (softTouched[i] == false) {
      if (difference > touchThreshold) {
        softTouched[i] = true; 
      } else {
        softBaseline[i] = (softBaseline[i] * (1.0f - BASELINE_UPDATE_ALPHA)) + (currentRaw * BASELINE_UPDATE_ALPHA);
      }
    } else {
      if (difference < releaseThreshold) {
        softTouched[i] = false; 
      }
    }
  }
}

void loop() {
  // --- Check for mode switches from the Serial Monitor ---
  if (Serial.available() > 0) {
    char c = Serial.read();
    
    if (c == '1' && !advancedMode) {
      Serial.println("\n>>> SWITCHING TO ADVANCED MODE (Software Detector + High Power) <<<");
      advancedMode = true;
      applyYarnAFE();
      delay(2000); // Let hardware settle
      // Seed the baselines for the software detector
      tcaSelect(MPR_MUX_CHANNEL);
      for(int i=0; i<12; i++) {
        softBaseline[i] = cap.filteredData(i);
        softTouched[i] = false;
      }
      Serial.println("Advanced Mode Ready. Yarn should now light up GREEN!");
      Serial.println("(Type '0' to return to Default Mode)");
      
    } else if (c == '0' && advancedMode) {
      Serial.println("\n>>> SWITCHING TO DEFAULT MODE (Hardware Math) <<<");
      advancedMode = false;
      resetToDefaults();
      Serial.println("Default Mode Ready. Yarn should now fail again.");
    }
  }

  bool bareIsTouched = false;
  bool yarnIsTouched = false;

  // --- Get Touch Data based on current mode ---
  if (advancedMode) {
    updateSoftwareDetector(); // Custom math
    bareIsTouched = softTouched[BARE_PIN];
    yarnIsTouched = softTouched[YARN_PIN];
  } else {
    tcaSelect(MPR_MUX_CHANNEL);
    uint16_t touched = cap.touched(); // Chip's default math
    bareIsTouched = (touched & (1 << BARE_PIN));
    yarnIsTouched = (touched & (1 << YARN_PIN));

    // Lightweight runtime debug: show touched bitmask + chosen electrode deltas.
    if (millis() - lastDebugPrint >= 250) {
      int bareDelta = (int)cap.baselineData(BARE_PIN) - (int)cap.filteredData(BARE_PIN);
      int yarnDelta = (int)cap.baselineData(YARN_PIN) - (int)cap.filteredData(YARN_PIN);
      Serial.print("touched=0b"); Serial.print(touched, BIN);
      Serial.print(" bareDelta="); Serial.print(bareDelta);
      Serial.print(" yarnDelta="); Serial.println(yarnDelta);
      lastDebugPrint = millis();
    }
  }

  // --- Visual Output to Matrix ---
  strip.clear(); // Turn off all LEDs to start fresh every loop
  
  if (bareIsTouched) {
    // Light up the first 128 LEDs BLUE
    for(int i = 0; i < LED_COUNT / 2; i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 255)); 
    }
  }
  
  if (yarnIsTouched) {
    // Light up the last 128 LEDs GREEN
    for(int i = LED_COUNT / 2; i < LED_COUNT; i++) {
      strip.setPixelColor(i, strip.Color(0, 255, 0)); 
    }
  }
  
  strip.show(); // Push the colors to the matrix hardware

  delay(50); // Keep the loop running at a sane speed
}