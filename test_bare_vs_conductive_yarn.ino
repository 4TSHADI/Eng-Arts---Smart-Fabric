#include <Wire.h>
#include "Adafruit_MPR121.h"

Adafruit_MPR121 cap = Adafruit_MPR121();

// Define which pins you are using for the test
const int BARE_ELECTRODE_PIN = 0;
const int YARN_ELECTRODE_PIN = 4;

void setup() {
  Serial.begin(115200);
  while (!Serial) { 
    delay(10); // Wait for serial port to open
  }
  
  Serial.println("Initializing MPR121 Test..."); 
  
  // Default I2C address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1); // Halt
  }
  
  Serial.println("MPR121 found!");
  delay(1000); // Give you a second to read the start message
  
  // Print the CSV Header. This is useful if you copy/paste to Excel.
  Serial.println("Bare_Filtered,Bare_Baseline,Yarn_Filtered,Yarn_Baseline");
}

void loop() {
  // 1. Read Filtered Data (The actual, current capacitance measurement)
  uint16_t bareFiltered = cap.filteredData(BARE_ELECTRODE_PIN);
  uint16_t yarnFiltered = cap.filteredData(YARN_ELECTRODE_PIN);
  
  // 2. Read Baseline Data (What the chip currently considers 'untouched')
  // Note: As mentioned in the text, this is a 10-bit value shifted left by 2 bits.
  uint16_t bareBaseline = cap.baselineData(BARE_ELECTRODE_PIN);
  uint16_t yarnBaseline = cap.baselineData(YARN_ELECTRODE_PIN);

  // 3. Print the data separated by commas for the Serial Plotter/CSV
  Serial.print(bareFiltered); Serial.print(",");
  Serial.print(bareBaseline); Serial.print(",");
  Serial.print(yarnFiltered); Serial.print(",");
  Serial.println(yarnBaseline);
  
  // A 50ms delay provides a smooth 20Hz update rate for graphing
  delay(50); 
}