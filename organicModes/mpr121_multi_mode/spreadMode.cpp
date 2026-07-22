#include "spreadMode.h"

void SpreadMode::enter(Adafruit_NeoPixel& strip) {
  strip.setBrightness(50); // Set standard brightness
  strip.clear();
  strip.show();
}

#define particleNUM 40

// Colour of spread
const uint8_t spreadr = 255;
const uint8_t spreadg = 255;
const uint8_t spreadb = 255;

// Point of origin for spread
const float spreadX = 8;
const float spreadY = 8;


// LED brightness
const uint8_t matrixBrightness = 90;

// Set so only goes once
bool spreadOut = false;

// Particle parameters
struct Particle
// For the particles (x start, y start, x speed/direction, y speed/direction, r value, g value, b value, brightness, on/off)
{
  float x;
  float y;

  float dx;
  float dy;

  uint8_t r;
  uint8_t g;
  uint8_t b;

  float brightness;

  bool active;
};

Particle particles[particleNUM];
static uint32_t noiseFrame = 0;

float fade(float t) {
  return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float lerp(float a, float b, float t) {
  return a + t * (b - a);
}

float fract(float x) {
  return x - floor(x);
}

float hash2D(float x, float y) {
  return fract(sin(x * 12.9898f + y * 78.233f) * 43758.5453f);
}

float perlinNoise2D(float x, float y) {
  int x0 = floor(x);
  int y0 = floor(y);
  float xf = x - x0;
  float yf = y - y0;

  float u = fade(xf);
  float v = fade(yf);

  float n00 = hash2D(x0, y0);
  float n10 = hash2D(x0 + 1, y0);
  float n01 = hash2D(x0, y0 + 1);
  float n11 = hash2D(x0 + 1, y0 + 1);

  float x1 = lerp(n00, n10, u);
  float x2 = lerp(n01, n11, u);
  return lerp(x1, x2, v) * 2.0f - 1.0f;
}

// Makes the trail effect for the objects
void trail(Adafruit_NeoPixel& strip)
{
  for(int i = 0; i < LEDnum; i++)
  {
    // Finds the main colour of the object
    uint32_t c = strip.getPixelColor(i);

    uint8_t r = (c >> 16) & 0xFF;
    uint8_t g = (c >> 8) & 0xFF;
    uint8_t b = c & 0xFF;

    // Dims the trailing colours
    r = r * 230 / 255;
    g = g * 230 / 255;
    b = b * 230 / 255;

    strip.setPixelColor(i, strip.Color(r, g, b));
  }
}

void spread(float x,float y)
{
  for(int i = 0; i < particleNUM; i++)
  {
    // angle of spread
    float angle = random(0,360) * PI / 180.0;

    // Can change size of spread
    float speed = random(30,100) / 100.0;

    particles[i].x = x;
    particles[i].y = y;

    particles[i].dx = cos(angle) * speed;
    particles[i].dy = sin(angle) * speed;

    particles[i].brightness = 50;

    particles[i].r = spreadr;
    particles[i].g = spreadg;
    particles[i].b = spreadb;

    particles[i].active = true;
  }
}

void SpreadMode::onTouch(Adafruit_NeoPixel& strip, uint8_t pin, bool isTouched, int16_t pressure) {
  Serial.println();
  Serial.print("Pressure "); Serial.print(pressure);
  Serial.println();
  Serial.print("Pin "); Serial.print(pin);
  Serial.println();
  if (!isTouched || pin >= 12 || spreadOut)
        return;

  Point p = centerCoordinates[pin];
  spread(p.x, p.y);
    spreadOut = true;

    }


void SpreadMode::update(Adafruit_NeoPixel& strip)
{
    if (!spreadOut)
        return;

    trail(strip);

    bool anyActive = false;

    for (int i = 0; i < particleNUM; i++)
    {
        if (!particles[i].active)
            continue;

        anyActive = true;

        particles[i].x += particles[i].dx;
        particles[i].y += particles[i].dy;

        particles[i].brightness -= 2;

        float noise = perlinNoise2D(particles[i].x * 0.18f + noiseFrame * 0.01f,
                                     particles[i].y * 0.18f + noiseFrame * 0.01f);
        particles[i].x += noise * 0.04f;
        particles[i].y += noise * 0.04f;

        int xx = round(particles[i].x);
        int yy = round(particles[i].y);

        if (xx < 0 || xx >= WIDTH || yy < 0 || yy >= HEIGHT)
        {
            particles[i].active = false;
            continue;
        }

        if (xx >= 0 && xx < WIDTH && yy >= 0 && yy < HEIGHT)
        {
            uint8_t r = particles[i].r * particles[i].brightness / 255;
            uint8_t g = particles[i].g * particles[i].brightness / 255;
            uint8_t b = particles[i].b * particles[i].brightness / 255;

            strip.setPixelColor(XY(xx, yy), strip.Color(r, g, b));
        }
        else
        {
            particles[i].active = false;
        }
        delay(5);
    }

    strip.show();
    noiseFrame++;

    if (!anyActive)
    {
        spreadOut = false;
        strip.clear();
        strip.show();
    }
}