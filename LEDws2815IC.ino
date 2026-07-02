#include

#define LED_PIN     6     // LED strip data pin
#define NUM_LEDS    60     // Number of LEDs
#define BRIGHTNESS  128    // Initial brightness (0-255)
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB    // Most WS2812 LEDs use GRB order

CRGB leds[NUM_LEDS];

// Effect mode enumeration
enum Effects {
  EFFECT_FLOW,     // Rainbow flow effect
  EFFECT_BLINK,    // Rainbow blink effect
  EFFECT_MARQUEE,  // Marquee effect
  EFFECT_COUNT
};

uint8_t currentEffect = EFFECT_FLOW;
unsigned long lastEffectChange = 0;
unsigned long lastUpdate = 0;

void setup() {
  delay(3000); // Startup safety delay
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  Serial.begin(115200);
}

// Rainbow flow effect
void flowEffect() {
  static uint8_t hue = 0;
  static uint8_t pos = 0;

  // Gradually fade out all LEDs
  fadeToBlackBy(leds, NUM_LEDS, 10);

  // Set a colored point at the current position
  leds[pos] = CHSV(hue, 255, 255);

  // Move to the next position
  pos = (pos + 1) % NUM_LEDS;

  // Change hue
  hue += 3;

  FastLED.show();
  delay(30);
}

// Rainbow blink effect
void blinkEffect() {
  static bool lightsOn = false;
  static uint8_t hue = 0;

  if(lightsOn) {
    // Fill the entire strip with a random color
    fill_solid(leds, NUM_LEDS, CHSV(hue, 255, 255));
    hue += 5;
  } else {
    // Turn all LEDs off
    fill_solid(leds, NUM_LEDS, CRGB::Black);
  }

  lightsOn = !lightsOn;
  FastLED.show();
  delay(lightsOn ? 200 : 800); // On for 200ms, off for 800ms
}

// Marquee effect
void marqueeEffect() {
  static uint8_t hue = 0;
  static int position = 0;
  static int tailLength = 15;

  // Gradually fade out LEDs
  fadeToBlackBy(leds, NUM_LEDS, 20);

  // Create the marquee head
  leds[position] = CHSV(hue, 255, 255);

  // Create gradient tail
  for(int i = 1; i  30000) {
    currentEffect = (currentEffect + 1) % EFFECT_COUNT;
    lastEffectChange = millis();
    FastLED.clear();
    FastLED.show();
  }

  // Run the current effect
  switch(currentEffect) {
    case EFFECT_FLOW:    flowEffect();    break;
    case EFFECT_BLINK:   blinkEffect();   break;
    case EFFECT_MARQUEE: marqueeEffect(); break;
  }

  // Optional: Brightness control
  // adjustBrightness();
}

// Optional: Switch effects via serial commands
void serialEvent() {
  while(Serial.available()) {
    char c = Serial.read();
    if(c &gt;= '0' &amp;&amp; c &lt;= &#039;2&#039;) {
      currentEffect = c - &#039;0&#039;;
      FastLED.clear();
      FastLED.show();
    }
  }
}

// Optional: Brightness adjustment function
void adjustBrightness() {
  int potValue = analogRead(A0); // Connect potentiometer to A0
  int newBrightness = map(potValue, 0, 1023, 0, 255);
  FastLED.setBrightness(newBrightness);
}