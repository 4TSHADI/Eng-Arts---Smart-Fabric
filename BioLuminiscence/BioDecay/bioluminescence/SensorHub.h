#ifndef SENSOR_HUB_H
#define SENSOR_HUB_H

#include <Wire.h>
#include "Adafruit_MPR121.h"
#include "DecayMode.h"

struct AxisTouch {
  bool valid;
  uint8_t cell;
  int16_t pressure;
};

class SensorHub {
public:
  SensorHub() {
    for (uint8_t i = 0; i < NUM_MUX_CHANNELS; i++) {
      _sensorReady[i] = false;
    }
    for (uint8_t panel = 0; panel < NUM_PANELS; panel++) {
      _wasTouched[panel] = false;
      _lastXCell[panel] = 0;
      _lastYCell[panel] = 0;
    }
  }

  void begin() {
    Wire.begin();
  }

  void initSensors() {
    for (uint8_t panel = 0; panel < NUM_PANELS; ++panel) {
      uint8_t xChannel = X_SENSOR_CHANNELS[panel];
      uint8_t yChannel = Y_SENSOR_CHANNELS[panel];

      char xLabel[20];
      char yLabel[20];
      snprintf(xLabel, sizeof(xLabel), "Panel %u X", panel);
      snprintf(yLabel, sizeof(yLabel), "Panel %u Y", panel);

      _sensorReady[xChannel] = initSensor(_sensors[xChannel], xChannel, xLabel);
      _sensorReady[yChannel] = initSensor(_sensors[yChannel], yChannel, yLabel);
    }
  }

  void pollTouches(DecayMode* activeMode, Adafruit_NeoPixel& strip) {
    if (activeMode == nullptr) return;

    for (uint8_t panel = 0; panel < NUM_PANELS; ++panel) {
      uint8_t xChannel = X_SENSOR_CHANNELS[panel];
      uint8_t yChannel = Y_SENSOR_CHANNELS[panel];

      if (!_sensorReady[xChannel] || !_sensorReady[yChannel]) {
        continue;
      }

      AxisTouch xTouch = getStrongestTouch(_sensors[xChannel], xChannel);
      AxisTouch yTouch = getStrongestTouch(_sensors[yChannel], yChannel);

      if (xTouch.valid && yTouch.valid) {
        uint8_t xCell = (uint8_t)constrain(xTouch.cell, 0, TOUCH_GRID_SIZE - 1);
        uint8_t yCell = (uint8_t)constrain(yTouch.cell, 0, TOUCH_GRID_SIZE - 1);
        int16_t pressure = (int16_t)constrain((xTouch.pressure + yTouch.pressure) / 2, 0, 255);

        if (!_wasTouched[panel] || xCell != _lastXCell[panel] || yCell != _lastYCell[panel]) {
          if (_wasTouched[panel]) {
            TouchEvent offEvent = {panel, _lastXCell[panel], _lastYCell[panel], false, 0};
            activeMode->onTouch(strip, offEvent);
          }

          TouchEvent onEvent = {panel, xCell, yCell, true, pressure};
          activeMode->onTouch(strip, onEvent);

          _lastXCell[panel] = xCell;
          _lastYCell[panel] = yCell;

          Serial.print("Panel ");
          Serial.print(panel);
          Serial.print(" touch -> (x=");
          Serial.print(xCell);
          Serial.print(", y=");
          Serial.print(yCell);
          Serial.print(") pressure=");
          Serial.println(pressure);
        }

        _wasTouched[panel] = true;
      } else if (_wasTouched[panel]) {
        TouchEvent offEvent = {panel, _lastXCell[panel], _lastYCell[panel], false, 0};
        activeMode->onTouch(strip, offEvent);
        _wasTouched[panel] = false;
      }
    }
  }

private:
  Adafruit_MPR121 _sensors[NUM_MUX_CHANNELS];
  bool _sensorReady[NUM_MUX_CHANNELS];

  bool _wasTouched[NUM_PANELS];
  uint8_t _lastXCell[NUM_PANELS];
  uint8_t _lastYCell[NUM_PANELS];

  void tcaSelect(uint8_t channel) {
    if (channel > 7) return;

    Wire.beginTransmission(TCA9548A_ADDR);
    Wire.write(1 << channel);
    Wire.endTransmission();
  }

  bool initSensor(Adafruit_MPR121& sensor, uint8_t channel, const char* label) {
    tcaSelect(channel);
    delay(10);

    if (!sensor.begin(MPR121_ADDR)) {
      Serial.print(label);
      Serial.println(" not found");
      return false;
    }

    sensor.setAutoconfig(true);
    sensor.setThresholds(8, 4);

    Serial.print(label);
    Serial.println(" found");
    return true;
  }

  AxisTouch getStrongestTouch(Adafruit_MPR121& sensor, uint8_t muxChannel) {
    AxisTouch result = {false, 0, 0};

    tcaSelect(muxChannel);
    delay(2);

    uint16_t touched = sensor.touched();

    for (uint8_t i = 0; i < TOUCH_GRID_SIZE; ++i) {
      if (!(touched & (1 << i))) continue;

      int16_t pressure = (int16_t)sensor.baselineData(i) - (int16_t)sensor.filteredData(i);
      if (pressure < 0) pressure = 0;

      if (!result.valid || pressure > result.pressure) {
        result.valid = true;
        result.cell = i;
        result.pressure = pressure;
      }
    }

    return result;
  }
};

#endif // SENSOR_HUB_H
