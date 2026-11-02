// =====================================================================================
//  encoders.cpp - counting how far each wheel has turned (see encoders.h)
// =====================================================================================

#include <Arduino.h>

#include "config.h"
#include "encoders.h"
#include "motors.h"

// On ESP32 boards, code that runs in an interrupt should be marked IRAM_ATTR.
// Other boards neither have nor need it.
#if defined(ESP32)
#define ENCODER_ISR IRAM_ATTR
#else
#define ENCODER_ISR
#endif

// With USE_ENCODERS on, every encoder pin needs a real pin number, not -1. This check
// stops the build before an upload.
constexpr bool encoderPinsFilledIn(int wheel) {
  return WHEEL_PINS[wheel].encA >= 0 && WHEEL_PINS[wheel].encB >= 0;
}
static_assert(!USE_ENCODERS ||
                  (encoderPinsFilledIn(0) && encoderPinsFilledIn(1) && encoderPinsFilledIn(2)),
              "an encoder pin is -1 in the pin table: fill it in, or set USE_ENCODERS to 0");

volatile long encoderCounts[WHEEL_COUNT];  // changed inside the interrupts below

long countAtLastCheck[WHEEL_COUNT];
float rpm[WHEEL_COUNT];
unsigned long lastSpeedCheckMs = 0;

// ---- The counting itself, run every time an encoder wire changes ----
//
// When A changes: if A and B now differ, the wheel moved forward one step; if they are
// the same, it moved back. When B changes, the rule is reversed. A drawing of the two
// square waves, a quarter of a cycle apart, shows why.

void ENCODER_ISR countFromA(int wheel) {
  if (digitalRead(WHEEL_PINS[wheel].encA) != digitalRead(WHEEL_PINS[wheel].encB)) {
    encoderCounts[wheel] = encoderCounts[wheel] + 1;
  } else {
    encoderCounts[wheel] = encoderCounts[wheel] - 1;
  }
}

void ENCODER_ISR countFromB(int wheel) {
  if (digitalRead(WHEEL_PINS[wheel].encA) == digitalRead(WHEEL_PINS[wheel].encB)) {
    encoderCounts[wheel] = encoderCounts[wheel] + 1;
  } else {
    encoderCounts[wheel] = encoderCounts[wheel] - 1;
  }
}

// A plain Arduino interrupt cannot be told which pin fired, so each wire has its own
// short function.
void ENCODER_ISR wheel1A() { countFromA(0); }
void ENCODER_ISR wheel1B() { countFromB(0); }
void ENCODER_ISR wheel2A() { countFromA(1); }
void ENCODER_ISR wheel2B() { countFromB(1); }
void ENCODER_ISR wheel3A() { countFromA(2); }
void ENCODER_ISR wheel3B() { countFromB(2); }

void setupEncoders() {
  if (!USE_ENCODERS) return;

  for (int wheel = 0; wheel < WHEEL_COUNT; wheel++) {
    pinMode(WHEEL_PINS[wheel].encA, INPUT_PULLUP);
    pinMode(WHEEL_PINS[wheel].encB, INPUT_PULLUP);
  }
  attachInterrupt(digitalPinToInterrupt(WHEEL_PINS[0].encA), wheel1A, CHANGE);
  attachInterrupt(digitalPinToInterrupt(WHEEL_PINS[0].encB), wheel1B, CHANGE);
  attachInterrupt(digitalPinToInterrupt(WHEEL_PINS[1].encA), wheel2A, CHANGE);
  attachInterrupt(digitalPinToInterrupt(WHEEL_PINS[1].encB), wheel2B, CHANGE);
  attachInterrupt(digitalPinToInterrupt(WHEEL_PINS[2].encA), wheel3A, CHANGE);
  attachInterrupt(digitalPinToInterrupt(WHEEL_PINS[2].encB), wheel3B, CHANGE);
}

long encoderCount(int wheel) {
  // Interrupts pause briefly so that the number is read in one piece.
  noInterrupts();
  long count = encoderCounts[wheel];
  interrupts();
  return WHEEL_SETUP[wheel].flipEncoder ? -count : count;
}

void resetEncoders() {
  noInterrupts();
  for (int wheel = 0; wheel < WHEEL_COUNT; wheel++) encoderCounts[wheel] = 0;
  interrupts();
  for (int wheel = 0; wheel < WHEEL_COUNT; wheel++) countAtLastCheck[wheel] = 0;
}

float wheelRpm(int wheel) {
  return rpm[wheel];
}

void updateWheelSpeeds(unsigned long now) {
  if (now - lastSpeedCheckMs < 100) return;

  float minutes = (now - lastSpeedCheckMs) / 60000.0;
  for (int wheel = 0; wheel < WHEEL_COUNT; wheel++) {
    long count = encoderCount(wheel);
    rpm[wheel] = (count - countAtLastCheck[wheel]) / COUNTS_PER_WHEEL_TURN / minutes;
    countAtLastCheck[wheel] = count;
  }
  lastSpeedCheckMs = now;
}

int encoderPinLevel(int wheel, bool channelA) {
  if (!USE_ENCODERS) return 0;
  return digitalRead(channelA ? WHEEL_PINS[wheel].encA : WHEEL_PINS[wheel].encB);
}
