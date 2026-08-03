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
const int BARE_PIN = 0;
const int YARN_PIN = 4;

static const uint8_t MPR121_ADDR = 0x5A;
static const uint8_t REG_ECR = 0x5E;
static const uint8_t REG_CONFIG1 = 0x5C;
static const uint8_t REG_CONFIG2 = 0x5D;

// --- Software Detector Variables ---
float softBaseline[12]; 
bool softTouched[12];
const int TOUCH_THRESHOLDS[12]   = {8, 15, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12}; 
const int RELEASE_THRESHOLDS[12] = {4, 10,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8}; 

bool advancedMode = false; // We start in the "broken" default state for demonstration

// Applies high power to the yarn
void applyYarnAFE() {
  Wire.beginTransmission(MPR121_ADDR); Wire.write(REG_ECR); Wire.write(0x00); Wire.endTransmission(); delay(2);
  Wire.beginTransmission(MPR121_ADDR); Wire.write(REG_CONFIG1); Wire.write(0x3F); Wire.endTransmission();
  Wire.beginTransmission(MPR121_ADDR); Wire.write(REG_CONFIG2); Wire.write(0x24); Wire.endTransmission();
  Wire.beginTransmission(MPR121_ADDR); Wire.write(REG_ECR); Wire.write(0x8C); Wire.endTransmission(); delay(2);
}

// Resets the MPR121 chip back to Adafruit's factory defaults
void resetToDefaults() {
  cap.begin(MPR121_ADDR);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  // Initialize Matrix
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  strip.setBrightness(40); // 40 is bright enough, prevents Arduino power crashes

  Serial.println("\n=== Interactive Matrix Demonstration ===");
  
  if (!cap.begin(MPR121_ADDR)) {
    Serial.println("MPR121 not found! Check wiring.");
    while(1);
  }
  
  Serial.println("\nStarting in DEFAULT MODE (No Configuration).");
  Serial.println("  -> The bare wire should light up BLUE.");
  Serial.println("  -> The yarn will likely fail to light up GREEN.");
  Serial.println("\nType '1' and press Enter to switch to ADVANCED MODE.");
}

void updateSoftwareDetector() {
  for (int i = 0; i < 12; i++) {
    int currentRaw = cap.filteredData(i);
    int difference = abs(currentRaw - (int)softBaseline[i]);
    
    if (softTouched[i] == false) {
      if (difference > TOUCH_THRESHOLDS[i]) {
        softTouched[i] = true; 
      } else {
        softBaseline[i] = (softBaseline[i] * 0.95) + (currentRaw * 0.05);
      }
    } else {
      if (difference < RELEASE_THRESHOLDS[i]) {
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
    uint16_t touched = cap.touched(); // Chip's default math
    bareIsTouched = (touched & (1 << BARE_PIN));
    yarnIsTouched = (touched & (1 << YARN_PIN));
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