#include <Wire.h>
#include <Adafruit_MPR121.h>

Adafruit_MPR121 cap = Adafruit_MPR121();

// Define your pins
const int BARE_PIN = 5;
const int YARN_PIN = 0;

// I2C Address and Registers for configuring the MPR121 AFE
static const uint8_t MPR121_ADDR = 0x5A;
static const uint8_t REG_ECR = 0x5E;
static const uint8_t REG_CONFIG1 = 0x5C;
static const uint8_t REG_CONFIG2 = 0x5D;

// Function to write directly to MPR121 registers
void mprWrite8(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPR121_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

// Function that forces the chip to use higher power for the yarn
void applyYarnAFE() {
  mprWrite8(REG_ECR, 0x00);
  delay(2);
  mprWrite8(REG_CONFIG1, 0x3F); // Max charge current
  mprWrite8(REG_CONFIG2, 0x24); // Longer charge time
  mprWrite8(REG_ECR, 0x8C);
  delay(2);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); } 

  Serial.println("\n=== MPR121 EXTENDED CALIBRATION TEST ===");
  
  if (!cap.begin(MPR121_ADDR)) {
    Serial.println("ERROR: MPR121 not found! Check wiring.");
    while (1); 
  }
  
  Serial.println("Sensor found. Applying Yarn AFE Profile...");
  applyYarnAFE(); 
  
  Serial.println("\n=== CALIBRATION PHASE ===");
  Serial.println("Letting hardware baseline slowly adapt. DO NOT TOUCH THE SENSORS!");
  
  // Extended 60-second calibration loop
  for (int i = 1; i > 0; i--) {
    Serial.print("Calibrating... ");
    Serial.print(i);
    Serial.println(" seconds remaining.");
    delay(1000); 
  }
  
  Serial.println("\n=== CALIBRATION COMPLETE. COPY DATA BELOW THIS LINE ===");
  Serial.println("Test_State,Bare_Filtered,Bare_Baseline,Yarn_Filtered,Yarn_Baseline");
}

// Helper function to collect data for a set amount of time
void collectData(const char* stateName, unsigned long durationMs) {
  unsigned long startTime = millis();
  while (millis() - startTime < durationMs) {
    uint16_t bFilt = cap.filteredData(BARE_PIN);
    uint16_t bBase = cap.baselineData(BARE_PIN);
    uint16_t yFilt = cap.filteredData(YARN_PIN);
    uint16_t yBase = cap.baselineData(YARN_PIN);

    // Print CSV row
    Serial.print(stateName); Serial.print(",");
    Serial.print(bFilt); Serial.print(",");
    Serial.print(bBase); Serial.print(",");
    Serial.print(yFilt); Serial.print(",");
    Serial.println(yBase);
    
    delay(50); // Collect 20 readings per second
  }
}

void loop() {
  // --- TEST 1: UNTOUCHED ---
  Serial.println("PREPARING: Hands off entirely!");
  delay(1000); 
  collectData("Untouched", 3000); 
  
  // --- TEST 2: BARE WIRE ---
  Serial.println("ACTION: TOUCH AND HOLD the BARE WIRE (Pin 0)...");
  delay(2000); 
  collectData("Touching_Bare", 3000); 
  
  // --- BREAK ---
  Serial.println("ACTION: RELEASE! Hands off.");
  delay(2000); 
  
  // --- TEST 3: YARN ---
  Serial.println("ACTION: TOUCH AND HOLD the YARN (Pin 1)...");
  delay(2000);
  collectData("Touching_Yarn", 3000);
  
  // --- FINISHED ---
  Serial.println("=== TEST COMPLETE. COPY DATA ABOVE THIS LINE ===");
  Serial.println("(To run the test again, press the RESET button on your Arduino)");
  
  while(true) {
    delay(1000); 
  }
}