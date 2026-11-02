// =====================================================================================
//  04_Drive_Any_Direction - the omni mathematics: one formula for every direction
// =====================================================================================
//
//  Function: type g and press Enter in the Serial Monitor (115200 baud). The robot runs a
//  slow demonstration: forward, back, left, right, turn left, turn right and a diagonal.
//  Each move lasts 1 s, then the robot stops and pauses for 1 s. For every move it prints
//  the power of each wheel, to show the mathematics at work. It then waits for the next g.
//
//  Before starting:
//    1. Enter the notes from lesson 03 in WHEEL_ANGLE and FLIP_MOTOR below.
//    2. Lift the robot for a first check. Then put it on the floor, front facing away from
//       you, with about a metre of clear space all round.
//
//  Values to adjust: SPEED, MOVE_MS, PAUSE_MS, and the moves in loop().
//  The guide for this lesson is README.md in this folder.
//
// =====================================================================================

// ---- Pins: DFRobot Romeo ESP32-S3 ---------------------------------------------------
//  For another board or motor driver, docs/08_Other_Boards.md shows what to change.
//                                M1    M3    M4
const int EN_PIN[3]        = {   12,    9,   47 };  // EN = speed (PWM), 0 = brake
const int PH_PIN[3]        = {   13,   10,   11 };  // PH = direction
const int SOCKET_NUMBER[3] = {    1,    3,    4 };  // for the messages

// ---- Your wheels: copy these from the notes of lesson 03 ----------------------------
//  WHEEL_ANGLE  where the wheel sits, in degrees, seen from above, anticlockwise
//               from the front: 0 = front, 90 = left, 180 = back, 270 = right
//  FLIP_MOTOR   true if the wheel pushed clockwise in lesson 03
//  (They are the same as angle and flipMotor in WHEEL_SETUP in config.h.)
//                                 M1      M3      M4
const float WHEEL_ANGLE[3] = {     60,    180,    300 };
const bool FLIP_MOTOR[3]   = {  false,  false,  false };

// ---- Settings -----------------------------------------------------------------------

const int SPEED = 30;                 // % of full power for the busiest wheel
const unsigned long MOVE_MS = 1000;   // how long each move lasts
const unsigned long PAUSE_MS = 1000;  // how long to stay stopped between moves

// ---- Motors -------------------------------------------------------------------------

// percent: -100 to 100, 0 = brake. With the right FLIP_MOTOR, a positive number always
// pushes the robot anticlockwise, whichever way round the motor is wired.
void setMotorPower(int wheel, int percent) {
  if (FLIP_MOTOR[wheel]) {
    percent = -percent;
  }
  if (percent >= 0) {
    digitalWrite(PH_PIN[wheel], HIGH);
  } else {
    digitalWrite(PH_PIN[wheel], LOW);
  }
  analogWrite(EN_PIN[wheel], abs(percent) * 255 / 100);  // 0..100% becomes 0..255
}

void stopAll() {
  for (int wheel = 0; wheel < 3; wheel++) {
    setMotorPower(wheel, 0);
  }
}

// ---- The omni mathematics -----------------------------------------------------------
//
//  forward  1 = forward, -1 = back
//  left     1 = left,    -1 = right
//  turn     1 = turn left (anticlockwise), -1 = turn right (clockwise)
//
//  Each wheel gets:   -sin(angle) x forward  +  cos(angle) x left  +  turn
//
//  A wheel can push only along its rolling direction, at right angles to the line from
//  the robot's centre to the wheel (its rollers let it slide freely sideways).
//  The sin and cos give the part of the requested direction that lies along that push.
//  All the powers are then scaled so that the busiest wheel gets exactly SPEED %, and the
//  others get their share of that.
void drive(float forward, float left, float turn) {
  float power[3];
  float busiest = 0;

  for (int wheel = 0; wheel < 3; wheel++) {
    float angle = radians(WHEEL_ANGLE[wheel]);  // sin() and cos() want radians, not degrees
    power[wheel] = -sin(angle) * forward + cos(angle) * left + turn;
    if (fabs(power[wheel]) > busiest) {
      busiest = fabs(power[wheel]);
    }
  }

  for (int wheel = 0; wheel < 3; wheel++) {
    int percent = 0;
    if (busiest > 0) {
      percent = round(power[wheel] / busiest * SPEED);
    }
    setMotorPower(wheel, percent);

    // Print the numbers, for comparison with the table in README.md.
    Serial.print("   M");
    Serial.print(SOCKET_NUMBER[wheel]);
    Serial.print(" ");
    if (percent > 0) Serial.print("+");
    Serial.print(percent);
    Serial.print("%");
  }
  Serial.println();
}

// Wait for ms milliseconds, but stop everything immediately if something is typed (for
// example, x and Enter). An Enter on its own does not count. Returns false if stopped.
bool waitOrStop(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    while (Serial.available() > 0) {
      char c = Serial.read();
      if (c != '\r' && c != '\n') {
        stopAll();
        Serial.println();
        Serial.println("Stopped. Type g and press Enter to run the demo again.");
        return false;
      }
    }
    delay(5);
  }
  return true;
}

// One move of the demonstration: drive for MOVE_MS, stop, then stay stopped for PAUSE_MS.
// Returns false if the demonstration was stopped.
bool demoMove(float forward, float left, float turn) {
  drive(forward, left, turn);
  bool keepGoing = waitOrStop(MOVE_MS);
  stopAll();
  return keepGoing && waitOrStop(PAUSE_MS);
}

// ---- Waiting for the start command --------------------------------------------------

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
  }
  stopAll();  // every motor starts stopped
}

void loop() {
  waitForGo();

  Serial.println();
  Serial.print("Demo at ");
  Serial.print(SPEED);
  Serial.print("%. Each move lasts ");
  Serial.print(MOVE_MS);
  Serial.print(" ms, then a ");
  Serial.print(PAUSE_MS);
  Serial.println(" ms pause.");

  // demoMove(forward, left, turn)
  Serial.print("Forward      ");
  if (!demoMove(1, 0, 0)) return;
  Serial.print("Back         ");
  if (!demoMove(-1, 0, 0)) return;
  Serial.print("Left         ");
  if (!demoMove(0, 1, 0)) return;
  Serial.print("Right        ");
  if (!demoMove(0, -1, 0)) return;
  Serial.print("Turn left    ");
  if (!demoMove(0, 0, 1)) return;
  Serial.print("Turn right   ");
  if (!demoMove(0, 0, -1)) return;
  Serial.print("Diagonal     ");
  if (!demoMove(1, 1, 0)) return;  // forward and left at once: towards the front left

  Serial.println("Done.");
}
