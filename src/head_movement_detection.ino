// ============================================================
//  Biomedical Assistive System — Head Movement Detection
//  ADXL335 + Arduino Uno
//  COMMON ANODE 7-segment + Serial Monitor
// ============================================================

const int X_PIN = A0;
const int Y_PIN = A1;

// ── 7-segment pins ──
const int SEG_A = 2;
const int SEG_B = 3;
const int SEG_C = 4;
const int SEG_D = 5;
const int SEG_E = 6;
const int SEG_F = 7;
const int SEG_G = 8;

const int FILTER_SIZE   = 6;
const int CALIB_SAMPLES = 200;
const int DEBOUNCE_MS   = 150;

const int UP_THRESHOLD    = 40;
const int DOWN_THRESHOLD  = 10;
const int RIGHT_THRESHOLD = 18;
const int LEFT_DX_MIN     = 4;
const int LEFT_DY_MIN     = 8;

int baselineX = 0;
int baselineY = 0;

int xBuffer[FILTER_SIZE];
int yBuffer[FILTER_SIZE];

int filterIndex = 0;

String lastDirection = "NEUTRAL --";
unsigned long lastChangeTime = 0;


// ============================================================
// 7-segment COMMON ANODE
// ============================================================

void segClear() {
  digitalWrite(SEG_A, HIGH);
  digitalWrite(SEG_B, HIGH);
  digitalWrite(SEG_C, HIGH);
  digitalWrite(SEG_D, HIGH);
  digitalWrite(SEG_E, HIGH);
  digitalWrite(SEG_F, HIGH);
  digitalWrite(SEG_G, HIGH);
}


// ---------- UP (U) ----------
void segU() {
  segClear();

  digitalWrite(SEG_B, LOW);
  digitalWrite(SEG_C, LOW);
  digitalWrite(SEG_D, LOW);
  digitalWrite(SEG_E, LOW);
  digitalWrite(SEG_F, LOW);
}


// ---------- DOWN (d) ----------
void segDown() {
  segClear();

  digitalWrite(SEG_B, LOW);
  digitalWrite(SEG_C, LOW);
  digitalWrite(SEG_D, LOW);
  digitalWrite(SEG_E, LOW);
  digitalWrite(SEG_G, LOW);
}


// ---------- LEFT (L) ----------
void segL() {
  segClear();

  digitalWrite(SEG_D, LOW);
  digitalWrite(SEG_E, LOW);
  digitalWrite(SEG_F, LOW);
}


// ---------- RIGHT (R-like) ----------
void segRight() {
  segClear();

  digitalWrite(SEG_A, LOW);
  digitalWrite(SEG_B, LOW);
  digitalWrite(SEG_C, LOW);
  digitalWrite(SEG_E, LOW);
  digitalWrite(SEG_F, LOW);
  digitalWrite(SEG_G, LOW);
}


// ---------- Neutral ----------
void segNeutral() {
  segClear();
  digitalWrite(SEG_G, LOW);
}


// ============================================================
// Display Mapping
// ============================================================

void showOnDisplay(String direction) {

  if (direction == "UP     ^^")
    segU();

  else if (direction == "DOWN   vv")
    segDown();

  else if (direction == "LEFT   <<")
    segL();

  else if (direction == "RIGHT  >>")
    segRight();

  else
    segNeutral();
}


// ============================================================
// Moving average filter
// ============================================================

int movingAverage(int* buf, int newVal) {

  buf[filterIndex] = newVal;

  long sum = 0;
  for (int i = 0; i < FILTER_SIZE; i++)
    sum += buf[i];

  return sum / FILTER_SIZE;
}


// ============================================================
// Calibration
// ============================================================

void calibrate() {

  Serial.println("Calibrating... keep head still");

  long sumX = 0;
  long sumY = 0;

  for (int i = 0; i < CALIB_SAMPLES; i++) {

    sumX += analogRead(X_PIN);
    sumY += analogRead(Y_PIN);

    delay(10);
  }

  baselineX = sumX / CALIB_SAMPLES;
  baselineY = sumY / CALIB_SAMPLES;

  for (int i = 0; i < FILTER_SIZE; i++) {

    xBuffer[i] = baselineX;
    yBuffer[i] = baselineY;
  }

  Serial.println("Calibration complete");
}


// ============================================================
// Direction detection (UNCHANGED)
// ============================================================

String getRawDirection(int dx, int dy) {

  if (dy >= UP_THRESHOLD)
    return "UP     ^^";

  if (dx >= RIGHT_THRESHOLD)
    return "RIGHT  >>";

  if (dx <= -LEFT_DX_MIN && dy >= LEFT_DY_MIN && dy < UP_THRESHOLD)
    return "LEFT   <<";

  if (dy >= DOWN_THRESHOLD && dy < UP_THRESHOLD && abs(dx) < 15)
    return "DOWN   vv";

  return "NEUTRAL --";
}


// ============================================================
// Debounce
// ============================================================

String getStableDirection(int dx, int dy) {

  String raw = getRawDirection(dx, dy);

  unsigned long now = millis();

  if ((now - lastChangeTime) < DEBOUNCE_MS)
    return lastDirection;

  if (raw != lastDirection) {

    lastDirection = raw;
    lastChangeTime = now;
  }

  return lastDirection;
}


// ============================================================
// Serial Output
// ============================================================

void printStatus(int filtX, int filtY, String direction) {

  Serial.print(millis());
  Serial.print(" | ");

  Serial.print(filtX);
  Serial.print(" | ");

  Serial.print(filtY);
  Serial.print(" | ");

  int dx = filtX - baselineX;
  int dy = filtY - baselineY;

  Serial.print(dx);
  Serial.print(" | ");
  Serial.print(dy);
  Serial.print(" | ");

  Serial.println(direction);
}


// ============================================================
// Setup
// ============================================================

void setup() {

  Serial.begin(115200);

  pinMode(SEG_A, OUTPUT);
  pinMode(SEG_B, OUTPUT);
  pinMode(SEG_C, OUTPUT);
  pinMode(SEG_D, OUTPUT);
  pinMode(SEG_E, OUTPUT);
  pinMode(SEG_F, OUTPUT);
  pinMode(SEG_G, OUTPUT);

  segClear();

  analogReference(DEFAULT);

  delay(500);

  calibrate();
}


// ============================================================
// Loop
// ============================================================

void loop() {

  int rawX = analogRead(X_PIN);
  int rawY = analogRead(Y_PIN);

  int filtX = movingAverage(xBuffer, rawX);
  int filtY = movingAverage(yBuffer, rawY);

  filterIndex = (filterIndex + 1) % FILTER_SIZE;

  int dx = filtX - baselineX;
  int dy = filtY - baselineY;

  String direction = getStableDirection(dx, dy);

  showOnDisplay(direction);

  printStatus(filtX, filtY, direction);

  delay(10);
}