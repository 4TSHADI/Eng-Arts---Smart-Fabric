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

#define TCA_ADDR_A     0x70   // host mpr1-8, channels 0-7
#define TCA_ADDR_B     0x71   // host mpr9-12, channels 0-3

#define MPR121_ADDR    0x5A

#define NUM_ELECTRODES_PER_AXIS 12

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

// face: 0=Bottom 1=Left 2=Front 3=Right 4=Back 5=Top

Adafruit_NeoPixel strip(NUM_LEDS_TOTAL, DATA_PIN, NEOPIXEL_TYPE);
Adafruit_MPR121 mpr[12];
bool mprReady[12];

float brightness[NUM_FACES][FACE_SIZE][FACE_SIZE];

struct TouchTrack {
  bool active;
  float smoothedRow;
  float smoothedCol;
};
TouchTrack tracks[NUM_FACES][MAX_TOUCHES];

uint8_t chipMuxAddr[12]    = { TCA_ADDR_A, TCA_ADDR_A, TCA_ADDR_A, TCA_ADDR_A,
                                TCA_ADDR_A, TCA_ADDR_A, TCA_ADDR_A, TCA_ADDR_A,
                                TCA_ADDR_B, TCA_ADDR_B, TCA_ADDR_B, TCA_ADDR_B };
uint8_t chipMuxChannel[12] = { 0, 1, 2, 3, 4, 5, 6, 7,   0, 1, 2, 3 };

// ---------------- electrode ----------------
struct ElectrodeSource {
  uint8_t chip;
  uint8_t offset;
};

// [face][0]=frontCol [face][1]=backCol [face][2]=frontRow [face][3]=backRow
ElectrodeSource elecSrc[NUM_FACES][4] = {
  { {0,6}, {2,6}, {1,6}, {3,6} },   // face0 Bottom
  { {0,0}, {8,0}, {7,6}, {4,0} },   // face1 Left
  { {1,0}, {9,0}, {4,6}, {5,0} },   // face2 Front
  { {2,0}, {10,0}, {5,6}, {6,0} },  // face3 Right
  { {3,0}, {11,0}, {6,6}, {7,0} },  // face4 Back
  { {8,6}, {10,6}, {9,6}, {11,6} }  // face5 Top
};

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

float chipStrength[12][12];

// ---------------- mux channel ----------------
void tcaSelect(uint8_t muxAddr, uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(muxAddr);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

// ---------------- LED mapping ----------------
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

void setup() {
  Serial.begin(9600);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 1500) { }

  Wire.begin();

  Serial.println("Initializing 12 MPR121 chips via two TCA9548A muxes...");

  for (uint8_t c = 0; c < 12; c++) {
    tcaSelect(chipMuxAddr[c], chipMuxChannel[c]);
    if (mpr[c].begin(MPR121_ADDR, &Wire)) {
      mpr[c].setAutoconfig(true);
      mpr[c].setThresholds(TOUCH_THRESH, RELEASE_THRESH);
      Serial.print("MPR"); Serial.print(c + 1); Serial.println(": found.");
      mprReady[c] = true;
    } else {
      Serial.print("MPR"); Serial.print(c + 1); Serial.println(": NOT found.");
      mprReady[c] = false;
    }
  }

  strip.begin();
  strip.setBrightness(150);
  strip.clear();
  strip.show();

  memset(brightness, 0, sizeof(brightness));
  for (int f = 0; f < NUM_FACES; f++)
    for (int t = 0; t < MAX_TOUCHES; t++)
      tracks[f][t].active = false;

  Serial.println("Cube multi-touch (up to 2 per face) light trail ready.");
}

void refreshChipStrengths() {
  for (uint8_t c = 0; c < 12; c++) {
    if (!mprReady[c]) continue;
    tcaSelect(chipMuxAddr[c], chipMuxChannel[c]);
    for (uint8_t p = 0; p < 12; p++) {
      chipStrength[c][p] = (float)mpr[c].baselineData(p) - (float)mpr[c].filteredData(p);
    }
  }
}

void buildAxisStrength(ElectrodeSource front, ElectrodeSource back, float* out) {
  for (int e = 0; e < 6; e++) out[e] = chipStrength[front.chip][front.offset + e];
  for (int e = 0; e < 6; e++) out[e + 6] = chipStrength[back.chip][back.offset + e];
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
  ElectrodeSource frontCol = elecSrc[face][0];
  ElectrodeSource backCol  = elecSrc[face][1];
  ElectrodeSource frontRow = elecSrc[face][2];
  ElectrodeSource backRow  = elecSrc[face][3];

  if (!mprReady[frontCol.chip] || !mprReady[backCol.chip] ||
      !mprReady[frontRow.chip] || !mprReady[backRow.chip]) return 0;

  float colStrength[NUM_ELECTRODES_PER_AXIS];
  float rowStrength[NUM_ELECTRODES_PER_AXIS];
  buildAxisStrength(frontCol, backCol, colStrength);
  buildAxisStrength(frontRow, backRow, rowStrength);

  Cluster colClusters[MAX_TOUCHES];
  Cluster rowClusters[MAX_TOUCHES];
  int numColClusters = findClusters(colStrength, NUM_ELECTRODES_PER_AXIS, colClusters);
  int numRowClusters = findClusters(rowStrength, NUM_ELECTRODES_PER_AXIS, rowClusters);

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

  brightness[face][y0][x0] = min(255.0f, brightness[face][y0][x0]);
  brightness[face][y0][x1] = min(255.0f, brightness[face][y0][x1]);
  brightness[face][y1][x0] = min(255.0f, brightness[face][y1][x0]);
  brightness[face][y1][x1] = min(255.0f, brightness[face][y1][x1]);
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
  if (brightness[neighborFace][r][c] > 255.0f) brightness[neighborFace][r][c] = 255.0f;
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
          uint32_t color = strip.gamma32(strip.ColorHSV(29000, 200, b));
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
