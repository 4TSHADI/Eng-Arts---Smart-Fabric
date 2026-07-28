#ifndef FLASHING_MODE_H
#define FLASHING_MODE_H

#include "LightingMode.h"
#include "ProjectConfig.h"

// FlashingMode blinks any touched regions continuously
class FlashingMode : public LightingMode {
private:
  uint16_t touchedPins[NUM_MUX_CHANNELS]; // Bitmask of currently touched electrodes for each channel
  unsigned long lastFlashTime;
  bool flashOn;
  const unsigned long flashInterval = 200; // Blink period (ms)

  void redraw(Adafruit_NeoPixel& strip);

public:
  void enter(Adafruit_NeoPixel& strip) override;
  void onTouch(Adafruit_NeoPixel& strip, uint8_t ch, uint8_t pin, bool isTouched) override;
  void update(Adafruit_NeoPixel& strip) override;
};

#endif
