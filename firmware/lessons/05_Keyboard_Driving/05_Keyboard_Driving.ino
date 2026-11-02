// =====================================================================================
//  05_Keyboard_Driving - drive the robot with keys typed in the Serial Monitor
// =====================================================================================
//
//    w / s   forward / back            1 2 3 4 5   speed 20, 30, 40, 50, 60%
//    a / d   left / right              x or space  stop
//    q / e   turn left / right         h or ?      show the keys again
//
//  Each key drives for about 1 s, then the robot stops automatically. Send the key again
//  to continue. Any other key stops the robot immediately.
//
//  In the Arduino IDE: open the Serial Monitor (115200 baud, New Line), type a key and
//  press Enter.
//
//  Before starting: enter the notes from lesson 03 in WHEEL_ANGLE and FLIP_MOTOR below,
//  and clear some floor space (or lift the wheels for a first run).
//
//  Values to adjust: MOVE_MS, START_SPEED, and the keys in handleKey().
//  The guide for this lesson is README.md in this folder.
//
// =====================================================================================

// ---- Pins: DFRobot Romeo ESP32-S3 ---------------------------------------------------
//  For another board or motor driver, docs/08_Other_Boards.md shows what to change.
//                                M1    M3    M4
const int EN_PIN[3]        = {   12,    9,   47 };  // EN = speed (PWM), 0 = brake
const int PH_PIN[3]        = {   13,   10,   11 };  // PH = direction

// ---- Your wheels: copy these from the notes of lesson 03 ----------------------------
//  WHEEL_ANGLE  where the wheel sits, in degrees, seen from above, anticlockwise
//               from the front: 0 = front, 90 = left, 180 = back, 270 = right
//  FLIP_MOTOR   true if the wheel pushed clockwise in lesson 03
//                                 M1      M3      M4
const float WHEEL_ANGLE[3] = {     60,    180,    300 };
const bool FLIP_MOTOR[3]   = {  false,  false,  false };

// ---- Settings -----------------------------------------------------------------------

const unsigned long MOVE_MS = 1000;  // each driving key keeps the robot going this long
const int START_SPEED = 30;          // % until another speed is selected with 1 to 5

// ---- The present state of the robot -------------------------------------------------

int speed = START_SPEED;        // % of full power for the busiest wheel
bool moving = false;            // true while a move is running
unsigned long moveStartMs = 0;  // when the last driving key arrived, from millis()

// ---- Motors (the same as lesson 04) -------------------------------------------------

// percent: -100 to 100, 0 = brake. Positive pushes the robot anticlockwise.
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
  moving = false;
}

// The omni mathematics from lesson 04: forward, left and turn each go from -1 to 1, and the
// busiest wheel gets exactly `speed` %. Every call starts the MOVE_MS timer again.
void drive(float forward, float left, float turn) {
  float power[3];
  float busiest = 0;

  for (int wheel = 0; wheel < 3; wheel++) {
    float angle = radians(WHEEL_ANGLE[wheel]);
    power[wheel] = -sin(angle) * forward + cos(angle) * left + turn;
    if (fabs(power[wheel]) > busiest) {
      busiest = fabs(power[wheel]);
    }
  }

  for (int wheel = 0; wheel < 3; wheel++) {
    int percent = 0;
    if (busiest > 0) {
      percent = round(power[wheel] / busiest * speed);
    }
    setMotorPower(wheel, percent);
  }

  moving = true;
  moveStartMs = millis();
}

// ---- Keys ---------------------------------------------------------------------------

void printHelp() {
  Serial.println();
  Serial.println("Keyboard driving. Type a key (in the Arduino IDE, then press Enter):");
  Serial.println("  w / s   forward / back         1 2 3 4 5   speed 20, 30, 40, 50, 60%");
  Serial.println("  a / d   left / right           x or space  stop");
  Serial.println("  q / e   turn left / right      h or ?      these keys again");
  Serial.print("Each key drives for ");
  Serial.print(MOVE_MS);
  Serial.print(" ms, then the robot stops. Speed now: ");
  Serial.print(speed);
  Serial.println("%");
}

void setSpeed(int percent) {
  speed = percent;
  Serial.print("speed ");
  Serial.print(speed);
  Serial.println("% (from the next key)");
}

void handleKey(char key) {
  switch (key) {
    case 'w': Serial.println("forward");    drive(1, 0, 0);  break;
    case 's': Serial.println("back");       drive(-1, 0, 0); break;
    case 'a': Serial.println("left");       drive(0, 1, 0);  break;
    case 'd': Serial.println("right");      drive(0, -1, 0); break;
    case 'q': Serial.println("turn left");  drive(0, 0, 1);  break;
    case 'e': Serial.println("turn right"); drive(0, 0, -1); break;

    case '1': setSpeed(20); break;
    case '2': setSpeed(30); break;
    case '3': setSpeed(40); break;
    case '4': setSpeed(50); break;
    case '5': setSpeed(60); break;

    case 'x':
    case ' ':
      stopAll();
      Serial.println("stop");
      break;

    case 'h':
    case '?':
      stopAll();  // no driving while the list is being read
      printHelp();
      break;

    case '\r':
    case '\n':
      // The Serial Monitor adds these when Enter is pressed. They are not keys, so they
      // must not stop the robot.
      break;

    default:
      // Any other key stops the robot: stopping is the safe default.
      stopAll();
      Serial.println("stop (that key isn't used, h shows the keys)");
      break;
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

  delay(2000);  // gives the Serial Monitor time to reconnect after the upload
  printHelp();
}

void loop() {
  if (Serial.available() > 0) {
    char key = tolower(Serial.read());  // so W works as well as w
    handleKey(key);
  }

  // The safety timer: a move stops automatically MOVE_MS after the last driving key.
  // If typing ends, or the cable comes out, the robot therefore stops as well.
  if (moving && millis() - moveStartMs >= MOVE_MS) {
    stopAll();
    Serial.println("stopped (send the key again to keep going)");
  }
}
