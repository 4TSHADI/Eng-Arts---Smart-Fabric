#ifndef SENSOR_HUB_H
#define SENSOR_HUB_H

#include <Wire.h>
#include "Adafruit_MPR121.h"
#include "LightingMode.h"

struct AxisTouch {
  bool valid;
  uint8_t cell;
  int16_t pressure;
};

class SensorHub {
public:
  SensorHub() {
    _muxAddr = 0;
    for (uint8_t i = 0; i < NUM_MUX_CHANNELS; i++) {
      _sensorReady[i] = false;
      _sensorAddr[i] = 0;
    }
    for (uint8_t panel = 0; panel < NUM_PANELS; panel++) {
      _wasTouched[panel] = false;
      _lastXCell[panel] = 0;
      _lastYCell[panel] = 0;
    }
  }

  void begin() {
#if I2C_FORCE_INTERNAL_PULLUPS
    pinMode(SDA, INPUT_PULLUP);
    pinMode(SCL, INPUT_PULLUP);
#endif

    Wire.begin();
    Wire.setClock(100000);
    Serial.println("I2C clock set to 100kHz");
    printI2CLineState();
  }

  void initSensors() {
    printI2CDiagnostics();

    if (_muxAddr == 0) {
      Serial.println("Sensor init aborted: TCA9548A mux not detected.");
      return;
    }

    for (uint8_t panel = 0; panel < NUM_PANELS; ++panel) {
      uint8_t yChannel = Y_SENSOR_CHANNELS[panel];
      uint8_t xChannel = X_SENSOR_CHANNELS[panel];

      char xLabel[20];
      char yLabel[20];
      snprintf(xLabel, sizeof(xLabel), "Panel %u X", panel);
      snprintf(yLabel, sizeof(yLabel), "Panel %u Y", panel);

      _sensorReady[xChannel] = initSensor(_sensors[xChannel], xChannel, xLabel);
      _sensorReady[yChannel] = initSensor(_sensors[yChannel], yChannel, yLabel);
    }
  }

  void pollTouches(LightingMode* activeMode, Adafruit_NeoPixel& strip) {
    if (activeMode == nullptr) return;

    for (uint8_t panel = 0; panel < NUM_PANELS; ++panel) {
      uint8_t yChannel = Y_SENSOR_CHANNELS[panel];
      uint8_t xChannel = X_SENSOR_CHANNELS[panel];

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

          if (TOUCH_EVENT_SERIAL_LOG) {
            Serial.print("Panel ");
            Serial.print(panel);
            Serial.print(" touch -> (x=");
            Serial.print(xCell);
            Serial.print(", y=");
            Serial.print(yCell);
            Serial.print(") pressure=");
            Serial.println(pressure);
          }
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
  static const uint16_t MUX_SETTLE_US = 150;

  Adafruit_MPR121 _sensors[NUM_MUX_CHANNELS];
  bool _sensorReady[NUM_MUX_CHANNELS];
  uint8_t _sensorAddr[NUM_MUX_CHANNELS];
  uint8_t _muxAddr;

  bool _wasTouched[NUM_PANELS];
  uint8_t _lastXCell[NUM_PANELS];
  uint8_t _lastYCell[NUM_PANELS];

  bool i2cAddressResponds(uint8_t addr) {
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();
    return err == 0;
  }

  void printI2CLineState() {
    int sdaState = digitalRead(SDA);
    int sclState = digitalRead(SCL);

    Serial.print("I2C line state: SDA=");
    Serial.print(sdaState == HIGH ? "HIGH" : "LOW");
    Serial.print(" SCL=");
    Serial.println(sclState == HIGH ? "HIGH" : "LOW");

    if (sdaState == LOW || sclState == LOW) {
      Serial.println("  Warning: SDA/SCL low at idle. Check wiring shorts, pullups, and mux RESET.");
    }
  }

  void printI2CDiagnostics() {
    Serial.println("I2C diagnostic scan:");

    Serial.print("  upstream scan: ");
    bool upstreamFoundAny = false;
    bool upstreamMprFound = false;
    for (uint8_t addr = 0x03; addr <= 0x77; ++addr) {
      if (i2cAddressResponds(addr)) {
        if (upstreamFoundAny) Serial.print(", ");
        Serial.print("0x");
        Serial.print(addr, HEX);
        upstreamFoundAny = true;
        if (addr >= 0x5A && addr <= 0x5D) {
          upstreamMprFound = true;
        }
      }
    }
    if (!upstreamFoundAny) {
      Serial.print("no devices");
    }
    Serial.println();

    if (upstreamMprFound) {
      Serial.println("  NOTE: MPR121 detected on upstream bus. Check mux-side wiring/channels.");
    }

    // Prefer configured default first, then scan the full TCA9548A range.
    _muxAddr = i2cAddressResponds(TCA9548A_ADDR) ? TCA9548A_ADDR : 0;
    if (_muxAddr == 0) {
      for (uint8_t addr = 0x70; addr <= 0x77; ++addr) {
        if (addr == TCA9548A_ADDR) continue;
        if (i2cAddressResponds(addr)) {
          _muxAddr = addr;
          break;
        }
      }
    }

    if (_muxAddr == 0) {
      Serial.println("  TCA9548A not found at 0x70-0x77");
      Serial.println("  If upstream scan also shows no devices, SDA/SCL/pullups/power/reset is the likely fault.");
      return;
    }

    Serial.print("  TCA9548A detected at 0x");
    Serial.println(_muxAddr, HEX);

    const uint8_t candidateAddrs[] = {0x5A, 0x5B, 0x5C, 0x5D};
    for (uint8_t ch = 0; ch < NUM_MUX_CHANNELS; ++ch) {
      if (!tcaSelect(ch)) {
        Serial.print("  mux ch");
        Serial.print(ch);
        Serial.println(": select failed");
        continue;
      }
      delayMicroseconds(MUX_SETTLE_US);

      Serial.print("  mux ch");
      Serial.print(ch);
      Serial.print(": ");

      bool any = false;
      for (uint8_t i = 0; i < sizeof(candidateAddrs); ++i) {
        uint8_t addr = candidateAddrs[i];
        if (i2cAddressResponds(addr)) {
          if (any) Serial.print(", ");
          Serial.print("0x");
          Serial.print(addr, HEX);
          any = true;
        }
      }

      if (!any) {
        Serial.print("no device at 0x5A-0x5D");
      }
      Serial.println();

      if (SENSOR_STARTUP_DEEP_SCAN) {
        Serial.print("  mux ch");
        Serial.print(ch);
        Serial.print(" full scan: ");
        bool foundAny = false;
        for (uint8_t addr = 0x03; addr <= 0x77; ++addr) {
          if (i2cAddressResponds(addr)) {
            if (foundAny) Serial.print(", ");
            Serial.print("0x");
            Serial.print(addr, HEX);
            foundAny = true;
          }
        }
        if (!foundAny) {
          Serial.print("no devices");
        }
        Serial.println();
      }
    }
  }

  bool tcaSelect(uint8_t channel) {
    if (channel > 7 || _muxAddr == 0) return false;

    Wire.beginTransmission(_muxAddr);
    Wire.write(1 << channel);
    return Wire.endTransmission() == 0;
  }

  bool initSensor(Adafruit_MPR121& sensor, uint8_t channel, const char* label) {
    if (!tcaSelect(channel)) {
      Serial.print(label);
      Serial.println(" mux select failed");
      return false;
    }
    delay(1);

    // MPR121 supports 4 possible I2C addresses based on ADDR pin strap.
    const uint8_t candidateAddrs[] = {0x5A, 0x5B, 0x5C, 0x5D};
    uint8_t foundAddr = 0;
    for (uint8_t i = 0; i < sizeof(candidateAddrs); ++i) {
      uint8_t addr = candidateAddrs[i];
      if (sensor.begin(addr)) {
        foundAddr = addr;
        break;
      }
    }

    if (foundAddr == 0) {
      Serial.print(label);
      Serial.println(" not found (tried 0x5A-0x5D)");
      return false;
    }

    _sensorAddr[channel] = foundAddr;

    sensor.setAutoconfig(true);
    sensor.setThresholds(MPR121_TOUCH_THRESHOLD, MPR121_RELEASE_THRESHOLD);

    Serial.print(label);
    Serial.print(" found at 0x");
    Serial.println(foundAddr, HEX);
    return true;
  }

  AxisTouch getStrongestTouch(Adafruit_MPR121& sensor, uint8_t muxChannel) {
    AxisTouch result = {false, 0, 0};

    if (!tcaSelect(muxChannel)) {
      return result;
    }
    delayMicroseconds(MUX_SETTLE_US);

    uint16_t touched = sensor.touched();

    for (uint8_t cell = 0; cell < TOUCH_GRID_SIZE; ++cell) {
      uint8_t electrode = SENSOR_ELECTRODE_MAP[muxChannel][cell];
      if (electrode == DISABLED_ELECTRODE || electrode >= ELECTRODES_PER_MPR) continue;
      if (!(touched & (1 << electrode))) continue;

      int16_t pressure = (int16_t)sensor.baselineData(electrode) - (int16_t)sensor.filteredData(electrode);
      if (pressure < 0) pressure = 0;

      if (!result.valid || pressure > result.pressure) {
        result.valid = true;
        result.cell = cell;
        result.pressure = pressure;
      }
    }

    return result;
  }
};

#endif // SENSOR_HUB_H
