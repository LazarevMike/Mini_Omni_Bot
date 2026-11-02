// =====================================================================================
//  01_Spin_One_Motor - make one wheel turn forward, stop, turn backward, stop, repeat
// =====================================================================================
//
//  Function: the motor in socket M1 turns forward for 2 s, stops for 1 s, turns backward
//  for 2 s, stops for 1 s, and starts again. The Serial Monitor (115200 baud) shows each
//  step.
//
//  Before uploading: rest the robot on a mug or a small box so that the wheels hang in the
//  air, connect the battery and switch the robot on. USB alone cannot turn the motors.
//
//  Values to adjust: SPEED (below) and the times in loop().
//  The guide for this lesson is README.md in this folder.
//
// =====================================================================================

// ---- Pins: DFRobot Romeo ESP32-S3 ---------------------------------------------------
//  For another board or motor driver, docs/08_Other_Boards.md shows what to change.

const int M1_EN_PIN = 12;  // EN = speed. Switched on and off very fast (PWM). 0 = brake.
const int M1_PH_PIN = 13;  // PH = direction. HIGH = forward, LOW = backward.

// ---- Settings -----------------------------------------------------------------------

// The motor's power: 0 = stopped, 255 = full power. 80 is about 30%.
const int SPEED = 80;

void setup() {
  Serial.begin(115200);

  pinMode(M1_PH_PIN, OUTPUT);
  pinMode(M1_EN_PIN, OUTPUT);
  analogWrite(M1_EN_PIN, 0);  // start stopped: EN at 0 = brake

  // A wait of a few seconds, to move hands clear and to let the Serial Monitor reconnect
  // after the upload.
  delay(3000);
  Serial.println("Lesson 01: forward 2 s, stop 1 s, backward 2 s, stop 1 s, and again.");
}

void loop() {
  Serial.println("Forward");
  digitalWrite(M1_PH_PIN, HIGH);  // PH HIGH: forward
  analogWrite(M1_EN_PIN, SPEED);  // EN pulsing: run at this power
  delay(2000);

  Serial.println("Stop");
  analogWrite(M1_EN_PIN, 0);      // EN at 0: brake
  delay(1000);

  Serial.println("Backward");
  digitalWrite(M1_PH_PIN, LOW);   // PH LOW: backward
  analogWrite(M1_EN_PIN, SPEED);
  delay(2000);

  Serial.println("Stop");
  analogWrite(M1_EN_PIN, 0);
  delay(1000);
}
