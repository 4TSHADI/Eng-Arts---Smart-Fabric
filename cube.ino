#include <Wire.h>
#include <Adafruit_MPR121.h>
#include <Adafruit_NeoPixel.h>

// ---------------- CONFIG ----------------
#define FACE_WIDTH     16
#define FACE_HEIGHT    16
#define NUM_FACES      6
#define PIXELS_PER_FACE (FACE_WIDTH * FACE_HEIGHT)
#define NUM_LEDS_TOTAL  (PIXELS_PER_FACE * NUM_FACES)

#define DATA_PIN       6
#define NEOPIXEL_TYPE  (NEO_GRB + NEO_KHZ800)

#define TCA9548A_ADDR  0x70
#define MPR121_I2C_ADDR 0x5A

#define NUM_COL_ELECTRODES 6
#define NUM_ROW_ELECTRODES 6
#define ROW_ELECTRODE_BASE 6

#define TOUCH_THRESH   16
#define RELEASE_THRESH 8

#define DECAY_FACTOR   0.85f
#define DEPOSIT_GAIN   255.0f
#define LOOP_BUDGET_MS 50

#define ELECTRODE_TO_LED_SCALE ((float)(FACE_WIDTH - 1) / (float)(NUM_COL_ELECTRODES - 1))

#define NOISE_FLOOR        30.0f
#define MIN_TOTAL_STRENGTH 60.0f
#define OUTPUT_SMOOTHING   0.5f

#define EDGE_MARGIN     2.0f   
#define BLEED_STRENGTH  0.4f   

// face: 0=Bottom 1=Left 2=Front 3=Right 4=Back 5=Top
// Bottom→Left→Front→Right→Back→Top

Adafruit_NeoPixel strip(NUM_LEDS_TOTAL, DATA_PIN, NEOPIXEL_TYPE);
Adafruit_MPR121 cap[NUM_FACES];
bool mprReady[NUM_FACES];

float brightness[NUM_FACES][FACE_HEIGHT][FACE_WIDTH];
float smoothedCol[NUM_FACES];
float smoothedRow[NUM_FACES];
 
// edge adjacency: 0=top 1=right 2=bottom 3=left
struct EdgeLink {
  int8_t neighborFace;
  uint8_t neighborEdge;
  bool reversed;
};

EdgeLink edgeMap[6][4] = {
  // face0 Bottom: top right bottom left
  { {1,2,false}, {2,2,true}, {3,2,true}, {4,2,false} },
  // face1 Left
  { {5,1,false}, {2,3,false}, {0,0,false}, {4,1,false} },
  // face2 Front
  { {5,0,false}, {3,3,false}, {0,1,true}, {1,1,false} },
  // face3 Right
  { {5,3,true}, {4,3,false}, {0,3,true}, {2,1,false} },
  // face4 Back
  { {5,2,false}, {1,3,false}, {0,0,false}, {3,1,false} },
  // face5 Top
  { {4,0,false}, {1,0,false}, {2,0,false}, {3,0,true} }
};

// ---------------- TCA9548A channel ----------------
void tcaSelect(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(TCA9548A_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

// ---------------- LED coordinate mapping ----------------
uint16_t faceXY(uint8_t x, uint8_t y) {
  if (y % 2 == 0) {
    return y * FACE_WIDTH + (FACE_WIDTH - 1 - x);
  } else {
    return y * FACE_WIDTH + x;
  }
}

uint16_t globalXY(uint8_t face, uint8_t x, uint8_t y) {
  return face * PIXELS_PER_FACE + faceXY(x, y);
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(9600);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 1500) { }

  Wire.begin();

  Serial.println("Initializing 6 faces via TCA9548A...");

  for (uint8_t f = 0; f < NUM_FACES; f++) {
    tcaSelect(f);
    if (cap[f].begin(MPR121_I2C_ADDR, &Wire)) {
      cap[f].setAutoconfig(true);
      cap[f].setThresholds(TOUCH_THRESH, RELEASE_THRESH);
      Serial.print("Face "); Serial.print(f); Serial.println(": MPR121 found.");
      mprReady[f] = true;
    } else {
      Serial.print("Face "); Serial.print(f); Serial.println(": MPR121 NOT found.");
      mprReady[f] = false;
    }
    smoothedCol[f] = -1;
    smoothedRow[f] = -1;
  }

  strip.begin();
  strip.setBrightness(150);
  strip.clear();
  strip.show();

  memset(brightness, 0, sizeof(brightness));

  Serial.println("Cube touch light trail ready (6 faces, edge bleed enabled).");
}

// ---------------- touch detect ----------------
bool getTouchPoint(uint8_t face, float &electrodeRow, float &electrodeCol) {
  if (!mprReady[face]) return false;

  tcaSelect(face);

  float colValueSum = 0, colWeightSum = 0;
  for (int e = 0; e < NUM_COL_ELECTRODES; e++) {
    float strength = (float)cap[face].baselineData(e) - (float)cap[face].filteredData(e);
    if (strength > NOISE_FLOOR) {
      colWeightSum += strength;
      colValueSum += strength * e;
    }
  }

  float rowValueSum = 0, rowWeightSum = 0;
  for (int e = 0; e < NUM_ROW_ELECTRODES; e++) {
    int idx = ROW_ELECTRODE_BASE + e;
    float strength = (float)cap[face].baselineData(idx) - (float)cap[face].filteredData(idx);
    if (strength > NOISE_FLOOR) {
      rowWeightSum += strength;
      rowValueSum += strength * e;
    }
  }

  if (colWeightSum < MIN_TOTAL_STRENGTH || rowWeightSum < MIN_TOTAL_STRENGTH) {
    smoothedCol[face] = -1;
    smoothedRow[face] = -1;
    return false;
  }

  float rawCol = colValueSum / colWeightSum;
  float rawRow = rowValueSum / rowWeightSum;

  if (smoothedCol[face] < 0) smoothedCol[face] = rawCol;
  else smoothedCol[face] = OUTPUT_SMOOTHING * rawCol + (1 - OUTPUT_SMOOTHING) * smoothedCol[face];

  if (smoothedRow[face] < 0) smoothedRow[face] = rawRow;
  else smoothedRow[face] = OUTPUT_SMOOTHING * rawRow + (1 - OUTPUT_SMOOTHING) * smoothedRow[face];

  electrodeCol = smoothedCol[face];
  electrodeRow = smoothedRow[face];

  if (electrodeCol < 0) electrodeCol = 0;
  if (electrodeCol > NUM_COL_ELECTRODES - 1) electrodeCol = NUM_COL_ELECTRODES - 1;
  if (electrodeRow < 0) electrodeRow = 0;
  if (electrodeRow > NUM_ROW_ELECTRODES - 1) electrodeRow = NUM_ROW_ELECTRODES - 1;

  return true;
}

void decayFaceBuffer(uint8_t face) {
  for (int y = 0; y < FACE_HEIGHT; y++) {
    for (int x = 0; x < FACE_WIDTH; x++) {
      brightness[face][y][x] *= DECAY_FACTOR;
      if (brightness[face][y][x] < 0.5f) brightness[face][y][x] = 0.0f;
    }
  }
}

void depositBilinear(uint8_t face, float ledRow, float ledCol) {
  int y0 = (int)floor(ledRow);
  int x0 = (int)floor(ledCol);
  int y1 = min(y0 + 1, FACE_HEIGHT - 1);
  int x1 = min(x0 + 1, FACE_WIDTH - 1);

  float fy = ledRow - y0;
  float fx = ledCol - x0;

  float w00 = (1 - fy) * (1 - fx);
  float w01 = (1 - fy) * fx;
  float w10 = fy * (1 - fx);
  float w11 = fy * fx;

  brightness[face][y0][x0] += DEPOSIT_GAIN * w00;
  brightness[face][y0][x1] += DEPOSIT_GAIN * w01;
  brightness[face][y1][x0] += DEPOSIT_GAIN * w10;
  brightness[face][y1][x1] += DEPOSIT_GAIN * w11;

  for (int y = 0; y < FACE_HEIGHT; y++)
    for (int x = 0; x < FACE_WIDTH; x++)
      if (brightness[face][y][x] > 255.0f) brightness[face][y][x] = 255.0f;
}

// neighbor infiltration
void depositEdgeBleed(uint8_t neighborFace, uint8_t neighborEdge, float alongEdge, float depthFromEdge, float strength) {
  float nRow, nCol;
  switch (neighborEdge) {
    case 0: nRow = depthFromEdge; nCol = alongEdge; break;
    case 1: nRow = alongEdge; nCol = FACE_WIDTH - 1 - depthFromEdge; break;
    case 2: nRow = FACE_HEIGHT - 1 - depthFromEdge; nCol = alongEdge; break;
    default: nRow = alongEdge; nCol = depthFromEdge; break;
  }
  int r = (int)nRow, c = (int)nCol;
  if (r < 0 || r >= FACE_HEIGHT || c < 0 || c >= FACE_WIDTH) return;
  brightness[neighborFace][r][c] += DEPOSIT_GAIN * strength;
  if (brightness[neighborFace][r][c] > 255.0f) brightness[neighborFace][r][c] = 255.0f;
}

void bleedAcrossEdges(uint8_t face, float ledRow, float ledCol) {
  if (ledRow < EDGE_MARGIN) {
    EdgeLink link = edgeMap[face][0];
    float alongEdge = link.reversed ? (FACE_WIDTH - 1 - ledCol) : ledCol;
    depositEdgeBleed(link.neighborFace, link.neighborEdge, alongEdge, ledRow, BLEED_STRENGTH);
  }
  if (ledRow > FACE_HEIGHT - 1 - EDGE_MARGIN) {
    EdgeLink link = edgeMap[face][2];
    float alongEdge = link.reversed ? (FACE_WIDTH - 1 - ledCol) : ledCol;
    depositEdgeBleed(link.neighborFace, link.neighborEdge, alongEdge, FACE_HEIGHT - 1 - ledRow, BLEED_STRENGTH);
  }
  if (ledCol < EDGE_MARGIN) {
    EdgeLink link = edgeMap[face][3];
    float alongEdge = link.reversed ? (FACE_HEIGHT - 1 - ledRow) : ledRow;
    depositEdgeBleed(link.neighborFace, link.neighborEdge, alongEdge, ledCol, BLEED_STRENGTH);
  }
  if (ledCol > FACE_WIDTH - 1 - EDGE_MARGIN) {
    EdgeLink link = edgeMap[face][1];
    float alongEdge = link.reversed ? (FACE_HEIGHT - 1 - ledRow) : ledRow;
    depositEdgeBleed(link.neighborFace, link.neighborEdge, alongEdge, FACE_WIDTH - 1 - ledCol, BLEED_STRENGTH);
  }
}

// ---------------- six faces ----------------
void renderAllFaces() {
  strip.clear();
  for (uint8_t f = 0; f < NUM_FACES; f++) {
    for (int y = 0; y < FACE_HEIGHT; y++) {
      for (int x = 0; x < FACE_WIDTH; x++) {
        uint8_t b = (uint8_t)brightness[f][y][x];
        if (b > 0) {
          uint32_t color = strip.gamma32(strip.ColorHSV(29000, 200, b));
          strip.setPixelColor(globalXY(f, x, y), color);
        }
      }
    }
  }
  strip.show();
}

// ---------------- main loop ----------------
void loop() {
  uint32_t frameStart = millis();

  for (uint8_t f = 0; f < NUM_FACES; f++) {
    float electrodeRow, electrodeCol;
    bool active = getTouchPoint(f, electrodeRow, electrodeCol);

    decayFaceBuffer(f);
    if (active) {
      float ledRow = electrodeRow * ELECTRODE_TO_LED_SCALE;
      float ledCol = electrodeCol * ELECTRODE_TO_LED_SCALE;
      depositBilinear(f, ledRow, ledCol);
      bleedAcrossEdges(f, ledRow, ledCol);
    }
  }

  renderAllFaces();

  uint32_t elapsed = millis() - frameStart;
  if (elapsed < LOOP_BUDGET_MS) {
    delay(LOOP_BUDGET_MS - elapsed);
  } else {
    Serial.print("Warning: frame took ");
    Serial.print(elapsed);
    Serial.println(" ms, over 50ms budget.");
  }
}