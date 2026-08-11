#ifndef MODE_CONTROLLER_H
#define MODE_CONTROLLER_H

#include "LightingMode.h"

struct ModeRegistration {
  const char* name;
  LightingMode* mode;
};

class ModeController {
public:
  ModeController() : _count(0), _activeIndex(0) {}

  bool registerMode(const char* name, LightingMode* mode) {
    if (_count >= MAX_MODES || mode == nullptr || name == nullptr) {
      return false;
    }

    _modes[_count].name = name;
    _modes[_count].mode = mode;
    _count++;
    return true;
  }

  void begin(Adafruit_NeoPixel& strip) {
    if (_count == 0) return;
    selectMode(0, strip);
  }

  uint8_t modeCount() const {
    return _count;
  }

  const char* modeName(uint8_t index) const {
    if (index >= _count) return "";
    return _modes[index].name;
  }

  uint8_t activeIndex() const {
    return _activeIndex;
  }

  bool selectMode(uint8_t index, Adafruit_NeoPixel& strip) {
    if (_count == 0 || index >= _count) return false;

    _activeIndex = index;
    _modes[_activeIndex].mode->enter(strip);

    Serial.print("Mode: ");
    Serial.println(_modes[_activeIndex].name);
    return true;
  }

  void printMenu() const {
    Serial.println("Choose a mode by typing one of these numbers:");
    for (uint8_t i = 0; i < _count; ++i) {
      Serial.print("  ");
      Serial.print(i + 1);
      Serial.print(" = ");
      Serial.println(_modes[i].name);
    }
    Serial.println("Type n for next mode or p for previous mode.");
  }

  void handleSerial(Adafruit_NeoPixel& strip) {
    while (Serial.available() > 0) {
      char c = (char)Serial.read();

      if (c >= '1' && c <= '9') {
        uint8_t index = (uint8_t)(c - '1');
        if (index < _count) {
          selectMode(index, strip);
        }
      } else if (c == 'n' || c == 'N') {
        if (_count > 0) {
          selectMode((uint8_t)((_activeIndex + 1) % _count), strip);
        }
      } else if (c == 'p' || c == 'P') {
        if (_count > 0) {
          selectMode((uint8_t)((_activeIndex + _count - 1) % _count), strip);
        }
      }
    }
  }

  LightingMode* activeMode() {
    if (_count == 0) return nullptr;
    return _modes[_activeIndex].mode;
  }

private:
  static const uint8_t MAX_MODES = 10;
  ModeRegistration _modes[MAX_MODES];
  uint8_t _count;
  uint8_t _activeIndex;
};

#endif // MODE_CONTROLLER_H
