#include <Wire.h>
#include "Adafruit_MPR121.h"

#define TCA9548A_ADDR 0x70
#define MUX_CHANNEL_X 0
#define MUX_CHANNEL_Y 1
#define MPR121_ADDR 0x5A

Adafruit_MPR121 xSensor;
Adafruit_MPR121 ySensor;

bool xReady = false;
bool yReady = false;

struct TouchResult {
  bool valid;
  int8_t pin;
  int16_t pressure;
};

void tcaSelect(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(TCA9548A_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

TouchResult getStrongestTouch(Adafruit_MPR121 &sensor, uint8_t muxChannel) {
  tcaSelect(muxChannel);
  delay(2);

  TouchResult result = {false, -1, -1};
  uint16_t touched = sensor.touched();

  for (uint8_t i = 0; i < 12; i++) {
    if (!(touched & (1 << i))) continue;

    int16_t pressure = (int16_t)sensor.baselineData(i) - (int16_t)sensor.filteredData(i);
    if (pressure < 0) pressure = 0;

    if (!result.valid || pressure > result.pressure) {
      result.valid = true;
      result.pin = (int8_t)i;
      result.pressure = pressure;
    }
  }

  return result;
}

void setup() {
  Serial.begin(9600);
  while (!Serial) { delay(10); }
  Wire.begin();

  Serial.println("Standalone 2-MPR XY coordinate scanner");
  Serial.println("One MPR121 = X axis, one MPR121 = Y axis");

  tcaSelect(MUX_CHANNEL_X);
  delay(10);
  if (xSensor.begin(MPR121_ADDR)) {
    xSensor.setAutoconfig(true);
    xSensor.setThresholds(12, 6);
    xReady = true;
    Serial.println("X sensor found on mux channel 0");
  } else {
    Serial.println("X sensor NOT found on mux channel 0");
  }

  tcaSelect(MUX_CHANNEL_Y);
  delay(10);
  if (ySensor.begin(MPR121_ADDR)) {
    ySensor.setAutoconfig(true);
    ySensor.setThresholds(12, 6);
    yReady = true;
    Serial.println("Y sensor found on mux channel 1");
  } else {
    Serial.println("Y sensor NOT found on mux channel 1");
  }
}

void loop() {
  if (!xReady || !yReady) {
    delay(100);
    return;
  }

  TouchResult xTouch = getStrongestTouch(xSensor, MUX_CHANNEL_X);
  TouchResult yTouch = getStrongestTouch(ySensor, MUX_CHANNEL_Y);

  if (xTouch.valid && yTouch.valid) {
    Serial.print("Most active point -> (x=");
    Serial.print(xTouch.pin);
    Serial.print(", y=");
    Serial.print(yTouch.pin);
    Serial.print(")  pressures: x=");
    Serial.print(xTouch.pressure);
    Serial.print(", y=");
    Serial.println(yTouch.pressure);
  } else {
    Serial.println("No touch detected");
  }

  delay(50);
}
