#include <Wire.h>
#include <Adafruit_MPR121.h>
#include <Adafruit_NeoPixel.h>

// ---------------- CONFIG ----------------
#define FACE_SIZE      16
#define NUM_FACES      6
#define PIXELS_PER_FACE (FACE_SIZE * FACE_SIZE)
#define NUM_LEDS_TOTAL  (PIXELS_PER_FACE * NUM_FACES)

#define DATA_PIN       6
#define NEOPIXEL_TYPE  (NEO_GRB + NEO_KHZ800)

#define TCA_ADDR_A     0x70
#define TCA_ADDR_B     0x71

#define MPR121_ADDR    0x5A

#define NUM_ELECTRODES_PER_AXIS 8
const uint8_t ELECTRODE_PINS[NUM_ELECTRODES_PER_AXIS] = {0, 1, 2, 3, 4, 5, 6, 11};

#define TOUCH_THRESH   16
#define RELEASE_THRESH 8

#define DECAY_FACTOR   0.85f
#define DEPOSIT_GAIN   255.0f
#define LOOP_BUDGET_MS 50

#define ELECTRODE_TO_LED_SCALE ((float)(FACE_SIZE - 1) / (float)(NUM_ELECTRODES_PER_AXIS - 1))

#define NOISE_FLOOR        30.0f
#define OUTPUT_SMOOTHING   0.5f

#define EDGE_MARGIN     2.0f
#define BLEED_STRENGTH  0.4f

#define MAX_TOUCHES 2

#define HEAT_PER_TOUCH   5.0f
#define HEAT_SATURATION  120.0f

// ---------------- MPR121 批量读取寄存器地址 ----------------
#define MPR121_REG_FILTDATA0L 0x04
#define MPR121_REG_BASELINE0  0x1E
#define MPR121_NUM_CHANNELS   13

// face编号: 0=Bottom 1=Left 2=Front 3=Right 4=Back 5=Top

Adafruit_NeoPixel strip(NUM_LEDS_TOTAL, DATA_PIN, NEOPIXEL_TYPE);
Adafruit_MPR121 mpr[12];
bool mprReady[12];

float brightness[NUM_FACES][FACE_SIZE][FACE_SIZE];
float heatCount[NUM_FACES][FACE_SIZE][FACE_SIZE];

struct TouchTrack {
  bool active;
  float smoothedRow;
  float smoothedCol;
};
TouchTrack tracks[NUM_FACES][MAX_TOUCHES];

uint8_t chipMuxAddr[12];
uint8_t chipMuxChannel[12];

#define colChip(face) ((face) * 2)
#define rowChip(face) ((face) * 2 + 1)

struct EdgeLink {
  int8_t neighborFace;
  uint8_t neighborEdge;
  bool reversed;
};

EdgeLink edgeMap[6][4] = {
  { {1,2,false}, {2,2,true}, {3,2,true}, {4,2,false} },
  { {5,1,false}, {2,3,false}, {0,0,false}, {4,1,false} },
  { {5,0,false}, {3,3,false}, {0,1,true}, {1,1,false} },
  { {5,3,true}, {4,3,false}, {0,3,true}, {2,1,false} },
  { {5,2,false}, {1,3,false}, {0,0,false}, {3,1,false} },
  { {4,0,false}, {1,0,false}, {2,0,false}, {3,0,true} }
};

float chipStrength[12][NUM_ELECTRODES_PER_AXIS];

void tcaSelect(uint8_t muxAddr, uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(muxAddr);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

uint16_t faceXY(uint8_t x, uint8_t y) {
  if (y % 2 == 0) {
    return y * FACE_SIZE + (FACE_SIZE - 1 - x);
  } else {
    return y * FACE_SIZE + x;
  }
}

uint16_t globalXY(uint8_t face, uint8_t x, uint8_t y) {
  return face * PIXELS_PER_FACE + faceXY(x, y);
}

uint16_t heatToHue(float heat) {
  float t = heat / HEAT_SATURATION;
  if (t > 1.0f) t = 1.0f;
  return (uint16_t)(43690.0f * (1.0f - t));
}

inline float clampBrightness(float v) {
  return v > 255.0f ? 255.0f : v;
}

// ---------------- 批量读取某颗芯片全部13个电极的filtered/baseline ----------------
bool mprBulkRead(uint8_t i2cAddr, uint16_t* filtered, uint16_t* baseline) {
  Wire.beginTransmission(i2cAddr);
  Wire.write(MPR121_REG_FILTDATA0L);
  if (Wire.endTransmission(false) != 0) return false;
  uint8_t n = Wire.requestFrom((int)i2cAddr, 26);
  if (n < 26) return false;
  for (uint8_t i = 0; i < MPR121_NUM_CHANNELS; i++) {
    uint8_t lo = Wire.read();
    uint8_t hi = Wire.read();
    filtered[i] = (uint16_t)lo | ((uint16_t)hi << 8);
  }

  Wire.beginTransmission(i2cAddr);
  Wire.write(MPR121_REG_BASELINE0);
  if (Wire.endTransmission(false) != 0) return false;
  n = Wire.requestFrom((int)i2cAddr, MPR121_NUM_CHANNELS);
  if (n < MPR121_NUM_CHANNELS) return false;
  for (uint8_t i = 0; i < MPR121_NUM_CHANNELS; i++) {
    baseline[i] = ((uint16_t)Wire.read()) << 2;
  }
  return true;
}

void setup() {
  Serial.begin(9600);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 1500) { }

  Wire.begin();

  for (uint8_t c = 0; c < 12; c++) {
    if (c < 8) {
      chipMuxAddr[c] = TCA_ADDR_A;
      chipMuxChannel[c] = c;
    } else {
      chipMuxAddr[c] = TCA_ADDR_B;
      chipMuxChannel[c] = c - 8;
    }
  }

  Serial.println("Initializing 12 MPR121 chips (2 per face) via two TCA9548A muxes...");

  for (uint8_t c = 0; c < 12; c++) {
    tcaSelect(chipMuxAddr[c], chipMuxChannel[c]);
    if (mpr[c].begin(MPR121_ADDR, &Wire)) {
      mpr[c].setAutoconfig(true);
      mpr[c].setThresholds(TOUCH_THRESH, RELEASE_THRESH);
      Serial.print("Chip "); Serial.print(c); Serial.println(": found.");
      mprReady[c] = true;
    } else {
      Serial.print("Chip "); Serial.print(c); Serial.println(": NOT found.");
      mprReady[c] = false;
    }
  }

  strip.begin();
  strip.setBrightness(150);
  strip.clear();
  strip.show();

  memset(brightness, 0, sizeof(brightness));
  memset(heatCount, 0, sizeof(heatCount));
  for (int f = 0; f < NUM_FACES; f++)
    for (int t = 0; t < MAX_TOUCHES; t++)
      tracks[f][t].active = false;

  Serial.println("Cube (6 faces, 8x8 electrodes each) heatmap + multi-touch ready.");
}

void refreshChipStrengths() {
  uint16_t filtered[MPR121_NUM_CHANNELS];
  uint16_t baseline[MPR121_NUM_CHANNELS];

  for (uint8_t c = 0; c < 12; c++) {
    if (!mprReady[c]) continue;
    tcaSelect(chipMuxAddr[c], chipMuxChannel[c]);
    if (!mprBulkRead(MPR121_ADDR, filtered, baseline)) continue;
    for (uint8_t p = 0; p < NUM_ELECTRODES_PER_AXIS; p++) {
      uint8_t pin = ELECTRODE_PINS[p];
      chipStrength[c][p] = (float)baseline[pin] - (float)filtered[pin];
    }
  }
}

struct Cluster {
  float centroid;
  float totalStrength;
};

int findClusters(float* strengths, int numElectrodes, Cluster* outClusters) {
  int count = 0;
  int i = 0;
  while (i < numElectrodes && count < MAX_TOUCHES) {
    if (strengths[i] > NOISE_FLOOR) {
      float valueSum = 0, weightSum = 0;
      while (i < numElectrodes && strengths[i] > NOISE_FLOOR) {
        valueSum += strengths[i] * i;
        weightSum += strengths[i];
        i++;
      }
      outClusters[count].centroid = valueSum / weightSum;
      outClusters[count].totalStrength = weightSum;
      count++;
    } else {
      i++;
    }
  }
  return count;
}

int detectTouches(uint8_t face, float* outRows, float* outCols) {
  uint8_t cc = colChip(face);
  uint8_t rc = rowChip(face);

  if (!mprReady[cc] || !mprReady[rc]) return 0;

  Cluster colClusters[MAX_TOUCHES];
  Cluster rowClusters[MAX_TOUCHES];
  int numColClusters = findClusters(chipStrength[cc], NUM_ELECTRODES_PER_AXIS, colClusters);
  int numRowClusters = findClusters(chipStrength[rc], NUM_ELECTRODES_PER_AXIS, rowClusters);

  int numTouches = min(numColClusters, numRowClusters);

  for (int a = 0; a < numColClusters; a++)
    for (int b = a + 1; b < numColClusters; b++)
      if (colClusters[b].totalStrength > colClusters[a].totalStrength) {
        Cluster tmp = colClusters[a]; colClusters[a] = colClusters[b]; colClusters[b] = tmp;
      }
  for (int a = 0; a < numRowClusters; a++)
    for (int b = a + 1; b < numRowClusters; b++)
      if (rowClusters[b].totalStrength > rowClusters[a].totalStrength) {
        Cluster tmp = rowClusters[a]; rowClusters[a] = rowClusters[b]; rowClusters[b] = tmp;
      }

  for (int t = 0; t < numTouches; t++) {
    outCols[t] = colClusters[t].centroid;
    outRows[t] = rowClusters[t].centroid;
  }

  return numTouches;
}

void decayFaceBuffer(uint8_t face) {
  for (int y = 0; y < FACE_SIZE; y++) {
    for (int x = 0; x < FACE_SIZE; x++) {
      brightness[face][y][x] *= DECAY_FACTOR;
      if (brightness[face][y][x] < 0.5f) brightness[face][y][x] = 0.0f;
    }
  }
}

void depositBilinear(uint8_t face, float ledRow, float ledCol) {
  int y0 = (int)floor(ledRow);
  int x0 = (int)floor(ledCol);
  int y1 = min(y0 + 1, FACE_SIZE - 1);
  int x1 = min(x0 + 1, FACE_SIZE - 1);

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

  brightness[face][y0][x0] = clampBrightness(brightness[face][y0][x0]);
  brightness[face][y0][x1] = clampBrightness(brightness[face][y0][x1]);
  brightness[face][y1][x0] = clampBrightness(brightness[face][y1][x0]);
  brightness[face][y1][x1] = clampBrightness(brightness[face][y1][x1]);

  heatCount[face][y0][x0] += HEAT_PER_TOUCH * w00;
  heatCount[face][y0][x1] += HEAT_PER_TOUCH * w01;
  heatCount[face][y1][x0] += HEAT_PER_TOUCH * w10;
  heatCount[face][y1][x1] += HEAT_PER_TOUCH * w11;
}

void depositEdgeBleed(uint8_t neighborFace, uint8_t neighborEdge, float alongEdge, float depthFromEdge, float strength) {
  float nRow, nCol;
  switch (neighborEdge) {
    case 0: nRow = depthFromEdge; nCol = alongEdge; break;
    case 1: nRow = alongEdge; nCol = FACE_SIZE - 1 - depthFromEdge; break;
    case 2: nRow = FACE_SIZE - 1 - depthFromEdge; nCol = alongEdge; break;
    default: nRow = alongEdge; nCol = depthFromEdge; break;
  }
  int r = (int)nRow, c = (int)nCol;
  if (r < 0 || r >= FACE_SIZE || c < 0 || c >= FACE_SIZE) return;
  brightness[neighborFace][r][c] += DEPOSIT_GAIN * strength;
  brightness[neighborFace][r][c] = clampBrightness(brightness[neighborFace][r][c]);
}

void bleedAcrossEdges(uint8_t face, float ledRow, float ledCol) {
  if (ledRow < EDGE_MARGIN) {
    EdgeLink link = edgeMap[face][0];
    float alongEdge = link.reversed ? (FACE_SIZE - 1 - ledCol) : ledCol;
    depositEdgeBleed(link.neighborFace, link.neighborEdge, alongEdge, ledRow, BLEED_STRENGTH);
  }
  if (ledRow > FACE_SIZE - 1 - EDGE_MARGIN) {
    EdgeLink link = edgeMap[face][2];
    float alongEdge = link.reversed ? (FACE_SIZE - 1 - ledCol) : ledCol;
    depositEdgeBleed(link.neighborFace, link.neighborEdge, alongEdge, FACE_SIZE - 1 - ledRow, BLEED_STRENGTH);
  }
  if (ledCol < EDGE_MARGIN) {
    EdgeLink link = edgeMap[face][3];
    float alongEdge = link.reversed ? (FACE_SIZE - 1 - ledRow) : ledRow;
    depositEdgeBleed(link.neighborFace, link.neighborEdge, alongEdge, ledCol, BLEED_STRENGTH);
  }
  if (ledCol > FACE_SIZE - 1 - EDGE_MARGIN) {
    EdgeLink link = edgeMap[face][1];
    float alongEdge = link.reversed ? (FACE_SIZE - 1 - ledRow) : ledRow;
    depositEdgeBleed(link.neighborFace, link.neighborEdge, alongEdge, FACE_SIZE - 1 - ledCol, BLEED_STRENGTH);
  }
}

void renderAllFaces() {
  strip.clear();
  for (uint8_t f = 0; f < NUM_FACES; f++) {
    for (int y = 0; y < FACE_SIZE; y++) {
      for (int x = 0; x < FACE_SIZE; x++) {
        uint8_t b = (uint8_t)brightness[f][y][x];
        if (b > 0) {
          uint16_t hue = heatToHue(heatCount[f][y][x]);
          uint32_t color = strip.gamma32(strip.ColorHSV(hue, 200, b));
          strip.setPixelColor(globalXY(f, x, y), color);
        }
      }
    }
  }
  strip.show();
}

void updateFaceTracks(uint8_t face) {
  float rawRows[MAX_TOUCHES], rawCols[MAX_TOUCHES];
  int numTouches = detectTouches(face, rawRows, rawCols);

  bool usedThisFrame[MAX_TOUCHES] = {false};
  for (int t = 0; t < MAX_TOUCHES; t++) {
    if (!tracks[face][t].active) continue;
    float bestDist = 1e9;
    int bestMatch = -1;
    for (int d = 0; d < numTouches; d++) {
      if (usedThisFrame[d]) continue;
      float dr = rawRows[d] - tracks[face][t].smoothedRow;
      float dc = rawCols[d] - tracks[face][t].smoothedCol;
      float dist = dr * dr + dc * dc;
      if (dist < bestDist) { bestDist = dist; bestMatch = d; }
    }
    if (bestMatch >= 0) {
      tracks[face][t].smoothedRow = OUTPUT_SMOOTHING * rawRows[bestMatch] + (1 - OUTPUT_SMOOTHING) * tracks[face][t].smoothedRow;
      tracks[face][t].smoothedCol = OUTPUT_SMOOTHING * rawCols[bestMatch] + (1 - OUTPUT_SMOOTHING) * tracks[face][t].smoothedCol;
      usedThisFrame[bestMatch] = true;
    } else {
      tracks[face][t].active = false;
    }
  }
  for (int d = 0; d < numTouches; d++) {
    if (usedThisFrame[d]) continue;
    for (int t = 0; t < MAX_TOUCHES; t++) {
      if (!tracks[face][t].active) {
        tracks[face][t].active = true;
        tracks[face][t].smoothedRow = rawRows[d];
        tracks[face][t].smoothedCol = rawCols[d];
        break;
      }
    }
  }
}

void loop() {
  uint32_t frameStart = millis();

  refreshChipStrengths();

  for (uint8_t f = 0; f < NUM_FACES; f++) {
    updateFaceTracks(f);

    decayFaceBuffer(f);
    for (int t = 0; t < MAX_TOUCHES; t++) {
      if (tracks[f][t].active) {
        float ledRow = tracks[f][t].smoothedRow * ELECTRODE_TO_LED_SCALE;
        float ledCol = tracks[f][t].smoothedCol * ELECTRODE_TO_LED_SCALE;
        depositBilinear(f, ledRow, ledCol);
        bleedAcrossEdges(f, ledRow, ledCol);
      }
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
