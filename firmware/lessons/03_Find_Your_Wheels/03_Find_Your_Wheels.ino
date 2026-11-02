// =====================================================================================
//  03_Find_Your_Wheels - run each motor on its own to find its position and push direction
// =====================================================================================
//
//  Function: type g and press Enter in the Serial Monitor (115200 baud). The robot runs
//  M1, then M3, then M4, each on its own at 25% for 1 s, and prints how far every encoder
//  counted. It then waits for the next g.
//
//  Watch each wheel and record:
//    1. its position on the robot (front left, back, front right...)
//    2. the direction it pushes the robot: anticlockwise or clockwise, seen from above
//    3. whether its encoder counted up or down (the robot prints this)
//  README.md in this folder converts those notes into the wheel settings: the same three
//  that later go in WHEEL_SETUP in firmware/OmniBots_Mini/config.h.
//
//  Before starting: lift the robot so that the wheels hang in the air, or put it on the
//  floor with about a metre of clear space around it. The battery must be connected and
//  the robot switched on.
//
//  Values to adjust: TEST_POWER, SPIN_MS, PAUSE_MS.
//
// =====================================================================================

// ---- Pins: DFRobot Romeo ESP32-S3 ---------------------------------------------------
//  For another board or motor driver, docs/08_Other_Boards.md shows what to change.
//  One column per motor socket:
//                                M1    M3    M4
const int EN_PIN[3]        = {   12,    9,   47 };  // EN = speed (PWM), 0 = brake
const int PH_PIN[3]        = {   13,   10,   11 };  // PH = direction, HIGH = forward
const int ENCODER_A_PIN[3] = {    4,    6,   15 };
const int ENCODER_B_PIN[3] = {    5,    7,   16 };

const int SOCKET_NUMBER[3] = {    1,    3,    4 };  // for the messages (M2 is not used)

// ---- Settings -----------------------------------------------------------------------

const int TEST_POWER = 25;            // % of full power. A negative value reverses all.
const unsigned long SPIN_MS = 1000;   // how long each motor runs
const unsigned long PAUSE_MS = 1500;  // time for the wheel to stop before the next one

const float COUNTS_PER_TURN = 1496;   // one wheel turn, from lesson 02

// ---- Encoders: the same counting as lesson 02, now for three wheels -----------------

// One count per wheel: slot 0 is M1, slot 1 is M3, slot 2 is M4.
volatile long encoderCount[3] = { 0, 0, 0 };

// ESP32 boards expect interrupt functions to be marked IRAM_ATTR (see below). Other boards
// do not recognise that word, so there it is defined as nothing and the sketch still builds.
#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

// When A changes: A and B now different = one step forward, the same = one step back.
// When B changes, the rule is reversed. IRAM_ATTR keeps these functions in the ESP32's
// fast internal memory, ready to run the moment a wire changes.
void IRAM_ATTR countFromA(int wheel) {
  if (digitalRead(ENCODER_A_PIN[wheel]) != digitalRead(ENCODER_B_PIN[wheel])) {
    encoderCount[wheel] = encoderCount[wheel] + 1;
  } else {
    encoderCount[wheel] = encoderCount[wheel] - 1;
  }
}

void IRAM_ATTR countFromB(int wheel) {
  if (digitalRead(ENCODER_A_PIN[wheel]) == digitalRead(ENCODER_B_PIN[wheel])) {
    encoderCount[wheel] = encoderCount[wheel] + 1;
  } else {
    encoderCount[wheel] = encoderCount[wheel] - 1;
  }
}

// attachInterrupt() cannot tell a function which wire changed, so each of the six wires
// has its own short function that passes on the wheel number.
void IRAM_ATTR m1AChanged() { countFromA(0); }
void IRAM_ATTR m1BChanged() { countFromB(0); }
void IRAM_ATTR m3AChanged() { countFromA(1); }
void IRAM_ATTR m3BChanged() { countFromB(1); }
void IRAM_ATTR m4AChanged() { countFromA(2); }
void IRAM_ATTR m4BChanged() { countFromB(2); }

// Read a count with the interrupts briefly paused, so that it arrives in one piece.
long readCount(int wheel) {
  noInterrupts();
  long count = encoderCount[wheel];
  interrupts();
  return count;
}

// ---- Motors -------------------------------------------------------------------------

// percent: 1 to 100 = forward, -1 to -100 = backward, 0 = brake.
void setMotorPower(int wheel, int percent) {
  if (percent >= 0) {
    digitalWrite(PH_PIN[wheel], HIGH);
  } else {
    digitalWrite(PH_PIN[wheel], LOW);
  }
  analogWrite(EN_PIN[wheel], abs(percent) * 255 / 100);  // 0..100% becomes 0..255
}

// ---- Helpers ------------------------------------------------------------------------

void printWheelName(int wheel) {
  Serial.print("M");
  Serial.print(SOCKET_NUMBER[wheel]);
}

// Wait until g is typed (and Enter pressed). Anything else is ignored. The reminder repeats
// every 3 s, because the first one is easy to miss while the Serial Monitor reconnects
// after an upload.
void waitForGo() {
  // Discard anything typed earlier.
  while (Serial.available() > 0) {
    Serial.read();
  }

  unsigned long lastReminder = millis() - 3000;
  while (true) {
    if (millis() - lastReminder >= 3000) {
      Serial.println("Ready. Wheels lifted (or floor clear)? Type g and press Enter to start.");
      lastReminder = millis();
    }
    if (Serial.available() > 0) {
      char c = Serial.read();
      if (c == 'g' || c == 'G') break;
    }
    delay(10);
  }

  // Let the rest of the line (the Enter) arrive, then discard it.
  delay(100);
  while (Serial.available() > 0) {
    Serial.read();
  }
}

// ---- Main ---------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);

  for (int wheel = 0; wheel < 3; wheel++) {
    pinMode(PH_PIN[wheel], OUTPUT);
    pinMode(EN_PIN[wheel], OUTPUT);
    setMotorPower(wheel, 0);  // every motor starts stopped

    pinMode(ENCODER_A_PIN[wheel], INPUT_PULLUP);
    pinMode(ENCODER_B_PIN[wheel], INPUT_PULLUP);
  }

  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN[0]), m1AChanged, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN[0]), m1BChanged, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN[1]), m3AChanged, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN[1]), m3BChanged, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN[2]), m4AChanged, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN[2]), m4BChanged, CHANGE);
}

void loop() {
  waitForGo();

  for (int wheel = 0; wheel < 3; wheel++) {
    // Store every count before the start, to show how far each one moved.
    long countBefore[3];
    for (int i = 0; i < 3; i++) {
      countBefore[i] = readCount(i);
    }

    Serial.println();
    printWheelName(wheel);
    Serial.print(" on its own at ");
    Serial.print(TEST_POWER);
    Serial.print("% for ");
    Serial.print(SPIN_MS);
    Serial.println(" ms. Where is it, and which way does it push?");

    setMotorPower(wheel, TEST_POWER);
    delay(SPIN_MS);
    setMotorPower(wheel, 0);  // brake
    delay(PAUSE_MS);          // let it stop completely before reading the counts

    // How far each encoder counted. Only the wheel that spun should show a big number.
    Serial.print("  counted:");
    for (int i = 0; i < 3; i++) {
      long counted = readCount(i) - countBefore[i];
      Serial.print("   ");
      printWheelName(i);
      Serial.print(" ");
      if (counted > 0) Serial.print("+");
      Serial.print(counted);
    }
    Serial.println();

    // The direction of this wheel's own encoder. Fewer than 20 counts is not reliable.
    long moved = readCount(wheel) - countBefore[wheel];
    Serial.print("  ");
    printWheelName(wheel);
    if (moved > 20) {
      Serial.print(" counts UP, about ");
      Serial.print(moved / COUNTS_PER_TURN);
      Serial.println(" turns");
    } else if (moved < -20) {
      Serial.print(" counts DOWN, about ");
      Serial.print(-moved / COUNTS_PER_TURN);
      Serial.println(" turns");
    } else {
      Serial.println(" hardly counted. Did it turn? If it did, check its encoder wires.");
    }
  }

  Serial.println();
  Serial.println("Done. README.md in this lesson's folder turns your notes into settings.");
}
