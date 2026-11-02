// =====================================================================================
//  motors.cpp - making the wheels turn (motors.h gives the summary)
// =====================================================================================

#include <Arduino.h>

#include "config.h"
#include "motors.h"

// A pin left at -1 in the pin table would make every motor update write to a pin that
// does not exist. This check stops the build before an upload: every driver needs a speed
// and a direction pin, and DRIVER_PWM_DIR needs "direction 2" (pin3) as well.
constexpr bool motorPinsFilledIn(int wheel) {
  return WHEEL_PINS[wheel].pin1 >= 0 && WHEEL_PINS[wheel].pin2 >= 0 &&
         (MOTOR_DRIVER != DRIVER_PWM_DIR || WHEEL_PINS[wheel].pin3 >= 0);
}
static_assert(motorPinsFilledIn(0) && motorPinsFilledIn(1) && motorPinsFilledIn(2),
              "a wheel's speed or direction pin is -1 in the pin table "
              "(DRIVER_PWM_DIR needs direction 2 as well)");

float wheelTarget[WHEEL_COUNT];  // what each wheel has been asked for, -1..1
float wheelPower[WHEEL_COUNT];   // what each wheel is getting right now, -1..1
int lastDuty[WHEEL_COUNT];       // the value last sent to each motor (-1 = nothing yet)
bool lastForward[WHEEL_COUNT];

// Send a power (-1..1) to one wheel's motor driver, right now.
void writeMotor(int wheel, float power) {
  const WheelPins &pins = WHEEL_PINS[wheel];
  if (WHEEL_SETUP[wheel].flipMotor) power = -power;

  bool forward = power >= 0;
  int duty = (int)(fabs(power) * PWM_MAX + 0.5);  // 0..255

  // If nothing changed, leave the pins alone. (On an ESP32, every analogWrite() call sets
  // up the whole speed signal again, so it is called only when necessary.)
  if (duty == lastDuty[wheel] && forward == lastForward[wheel]) return;
  lastDuty[wheel] = duty;
  lastForward[wheel] = forward;

  if (MOTOR_DRIVER == DRIVER_PH_EN) {
    if (duty == 0) {
      // EN at 0 brakes the motor. PH also goes low: a driver board whose PMODE pin is not
      // on GND (and so is in another mode) then stops as well, instead of running at full
      // power.
      analogWrite(pins.pin1, 0);
      digitalWrite(pins.pin2, LOW);
    } else {
      // PH selects the direction, EN carries the speed.
      digitalWrite(pins.pin2, forward ? HIGH : LOW);
      analogWrite(pins.pin1, duty);
    }
  } else if (MOTOR_DRIVER == DRIVER_IN1_IN2) {
    if (duty == 0) {
      // Both pins high brakes the motor (on the DRV8833 and DRV8871, for example).
      analogWrite(pins.pin1, PWM_MAX);
      analogWrite(pins.pin2, PWM_MAX);
    } else {
      // Pulse IN1 to go forward, IN2 to go backward. The other pin stays low.
      analogWrite(forward ? pins.pin2 : pins.pin1, 0);
      analogWrite(forward ? pins.pin1 : pins.pin2, duty);
    }
  } else {
    // DRIVER_PWM_DIR: IN1 and IN2 select the direction, PWM carries the speed.
    if (duty == 0) {
      // IN1 and IN2 both high, with PWM fully on, brake the motor on the TB6612FNG and on
      // the L298N. (Removing PWM alone would let an L298N coast, and if its ENA jumper were
      // left on, the motor would keep running.)
      digitalWrite(pins.pin2, HIGH);
      digitalWrite(pins.pin3, HIGH);
      analogWrite(pins.pin1, PWM_MAX);
    } else {
      analogWrite(pins.pin1, duty);
      digitalWrite(pins.pin2, forward ? HIGH : LOW);
      digitalWrite(pins.pin3, forward ? LOW : HIGH);
    }
  }
}

// Make the speed signal fast enough to be silent. Every Arduino core does this slightly
// differently, and the Arduino Mega cannot change it at all (a faint whine is normal there).
void setPwmFrequency(int pin) {
#if defined(ESP32) && ESP_ARDUINO_VERSION_MAJOR >= 3
  analogWriteFrequency(pin, MOTOR_PWM_HZ);
#elif defined(ESP32) || defined(ARDUINO_ARCH_STM32)
  analogWriteFrequency(MOTOR_PWM_HZ);
  (void)pin;
#else
  (void)pin;
#endif
}

void setupMotors() {
  for (int wheel = 0; wheel < WHEEL_COUNT; wheel++) {
    const WheelPins &pins = WHEEL_PINS[wheel];
    pinMode(pins.pin1, OUTPUT);
    pinMode(pins.pin2, OUTPUT);
    // Only DRIVER_PWM_DIR uses pin3. (The presets keep one ready for it, unused otherwise.)
    if (MOTOR_DRIVER == DRIVER_PWM_DIR) pinMode(pins.pin3, OUTPUT);

    // Start the speed signal at 0 (stopped), then set its fast, silent frequency.
    analogWrite(pins.pin1, 0);
    setPwmFrequency(pins.pin1);
    if (MOTOR_DRIVER == DRIVER_IN1_IN2) {
      analogWrite(pins.pin2, 0);
      setPwmFrequency(pins.pin2);
    }

    wheelTarget[wheel] = 0;
    wheelPower[wheel] = 0;
    lastDuty[wheel] = -1;  // ensures that the line below sends the value
    writeMotor(wheel, 0);
  }
}

void setWheelTarget(int wheel, float power) {
  if (power > 1) power = 1;
  if (power < -1) power = -1;
  wheelTarget[wheel] = power;
}

void stopMotorsNow() {
  for (int wheel = 0; wheel < WHEEL_COUNT; wheel++) {
    wheelTarget[wheel] = 0;
    wheelPower[wheel] = 0;
    writeMotor(wheel, 0);
  }
}

void updateMotors() {
  // Called every 10 ms, so this is the largest change of power allowed in one call.
  float step = 0.01 / SPEED_UP_TIME_S;

  for (int wheel = 0; wheel < WHEEL_COUNT; wheel++) {
    float gap = wheelTarget[wheel] - wheelPower[wheel];
    if (gap > step) gap = step;
    if (gap < -step) gap = -step;
    wheelPower[wheel] += gap;
    writeMotor(wheel, wheelPower[wheel]);
  }
}

bool motorsBusy() {
  for (int wheel = 0; wheel < WHEEL_COUNT; wheel++) {
    if (wheelTarget[wheel] != 0 || wheelPower[wheel] != 0) return true;
  }
  return false;
}
