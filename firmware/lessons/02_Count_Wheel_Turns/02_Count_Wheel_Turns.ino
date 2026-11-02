// =====================================================================================
//  02_Count_Wheel_Turns - read M1's encoder and print how far the wheel has turned
// =====================================================================================
//
//  Function: turn the M1 wheel slowly by hand. Five times a second the Serial Monitor
//  (115200 baud) shows the count and the number of wheel turns it represents. One direction
//  counts up and the other direction counts down.
//
//  The motor stays off the whole time, so the USB cable provides all the power needed.
//
//  Values to adjust: COUNTS_PER_TURN for different motors, PRINT_EVERY_MS.
//  The guide for this lesson is README.md in this folder.
//
// =====================================================================================

// ---- Pins: DFRobot Romeo ESP32-S3 ---------------------------------------------------
//  For another board, docs/08_Other_Boards.md shows what to change.
//  The encoder's power wire (Enc +) goes to 3V3, never 5 V: these pins cannot take 5 V.

const int M1_ENCODER_A_PIN = 4;  // encoder wire A
const int M1_ENCODER_B_PIN = 5;  // encoder wire B
const int M1_EN_PIN = 12;        // motor speed: held at 0 so that the motor stays off

// ---- Settings -----------------------------------------------------------------------

// Counts in one full turn of the wheel: 11 pulses per motor turn, x 34 for the gearbox,
// x 4 because every change on both wires is counted. 11 x 34 x 4 = 1496.
const float COUNTS_PER_TURN = 1496;

const unsigned long PRINT_EVERY_MS = 200;  // how often to print, in milliseconds

// ---- Counting -----------------------------------------------------------------------

// "volatile" tells the compiler that this number can change at any moment (the interrupt
// functions below change it), so the program must always read its present value.
volatile long m1Count = 0;

// These two functions run automatically every time an encoder wire changes: that is an
// interrupt. IRAM_ATTR keeps them in the ESP32's fast internal memory, ready to run at any
// moment.
//
// The rule: when A changes, compare it with B. If A and B now differ, the wheel moved one
// step forward. If they are the same, it moved one step back. When B changes, the rule is
// reversed.

// ESP32 boards expect interrupt functions to be marked IRAM_ATTR (see below). Other boards
// do not recognise that word, so there it is defined as nothing and the sketch still builds.
#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

void IRAM_ATTR m1EncoderAChanged() {
  if (digitalRead(M1_ENCODER_A_PIN) != digitalRead(M1_ENCODER_B_PIN)) {
    m1Count = m1Count + 1;
  } else {
    m1Count = m1Count - 1;
  }
}

void IRAM_ATTR m1EncoderBChanged() {
  if (digitalRead(M1_ENCODER_A_PIN) == digitalRead(M1_ENCODER_B_PIN)) {
    m1Count = m1Count + 1;
  } else {
    m1Count = m1Count - 1;
  }
}

void setup() {
  Serial.begin(115200);

  // Keep the motor off. (EN at 0 also means brake: with the battery on, the wheel is
  // slightly stiffer to turn by hand. This is normal.)
  pinMode(M1_EN_PIN, OUTPUT);
  digitalWrite(M1_EN_PIN, LOW);

  // INPUT_PULLUP holds each pin at 3.3 V through a weak resistor unless the encoder pulls
  // it down, so a pin never floats (for example, if a wire comes loose).
  pinMode(M1_ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(M1_ENCODER_B_PIN, INPUT_PULLUP);

  // Run the counting functions whenever A or B changes, up or down (CHANGE).
  attachInterrupt(digitalPinToInterrupt(M1_ENCODER_A_PIN), m1EncoderAChanged, CHANGE);
  attachInterrupt(digitalPinToInterrupt(M1_ENCODER_B_PIN), m1EncoderBChanged, CHANGE);
}

void loop() {
  // Copy the count with the interrupts briefly paused, so that it is read in one piece.
  noInterrupts();
  long count = m1Count;
  interrupts();

  Serial.print("M1: ");
  Serial.print(count);
  Serial.print(" counts   ");
  Serial.print(count / COUNTS_PER_TURN);  // shown with 2 decimals
  Serial.print(" turns   A=");
  Serial.print(digitalRead(M1_ENCODER_A_PIN));
  Serial.print(" B=");
  Serial.println(digitalRead(M1_ENCODER_B_PIN));

  // The interrupts continue counting during this wait.
  delay(PRINT_EVERY_MS);
}
