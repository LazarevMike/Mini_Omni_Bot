// Checks what motors.cpp puts on the driver pins for each kind of motor driver: forward,
// backward and, most of all, stopped. run.sh builds it once per MOTOR_DRIVER, on the
// ESP32-S3 DevKitC-1 preset (it has a pin3 for DRIVER_PWM_DIR).
//
// "What the motor sees" comes from each chip's truth table (DRV8876: TI SLVSDS7B tables
// 3-5; TB6612FNG and L298: their datasheets), including common wiring mistakes:
// a DRV8876 board with PMODE not on GND, and an L298N with its ENA jumper left on.
#include "Arduino.h"
uint32_t g_ms = 0;
void simAdvance(uint32_t ms) { g_ms += ms; }
std::map<int, int> g_pinLevel, g_pinInput, g_analog;
std::map<int, void (*)()> g_isr;
MockSerial Serial;

#include "motors.cpp"

int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s  (%s)\n", msg, #cond); failures++; } else printf("ok:   %s\n", msg); } while (0)

// What one wheel's motor receives. A speed pin counts as "on" whenever it is above 0 (the worst
// case for a stop).
enum Drive { BRAKE, COAST, FORWARD, REVERSE };
const char *NAMES[] = {"brake", "coast", "forward", "reverse"};

int level(int pin) { return g_pinLevel.count(pin) ? g_pinLevel[pin] : 0; }
int pwm(int pin) { return g_analog.count(pin) ? g_analog[pin] : 0; }

// DRV8876 in PH/EN mode (PMODE on GND), PWM mode (PMODE high), half-bridge mode (floating)
Drive drv8876(const WheelPins &w, int pmode) {
  bool en = pwm(w.pin1) > 0, ph = level(w.pin2);
  if (pmode == 0) return !en ? BRAKE : ph ? FORWARD : REVERSE;           // table 3
  if (pmode == 1) return (!en && !ph) ? COAST : (en && ph) ? BRAKE : en ? FORWARD : REVERSE;  // table 4
  // table 5: OUT1 follows EN/IN1, OUT2 follows PH/IN2
  return en == ph ? BRAKE : en ? FORWARD : REVERSE;
}
// DRV8871 (IN1/IN2): 0/0 coast, 1/1 brake
Drive drv8871(const WheelPins &w) {
  bool a = pwm(w.pin1) > 0, b = pwm(w.pin2) > 0;
  return (a && b) ? BRAKE : (!a && !b) ? COAST : a ? FORWARD : REVERSE;
}
// TB6612FNG: IN1=IN2=H short brake; PWM low short brake; else by IN1/IN2
Drive tb6612(const WheelPins &w) {
  bool in1 = level(w.pin2), in2 = level(w.pin3), p = pwm(w.pin1) > 0;
  if (in1 && in2) return BRAKE;
  if (!in1 && !in2) return COAST;
  if (!p) return BRAKE;
  return in1 ? FORWARD : REVERSE;
}
// L298N: EN low = free running (coast); C = D fast stop (brake). enTiedHigh = jumper left on.
Drive l298n(const WheelPins &w, bool enTiedHigh) {
  bool en = enTiedHigh || pwm(w.pin1) > 0, c = level(w.pin2), d = level(w.pin3);
  if (!en) return COAST;
  if (c == d) return BRAKE;
  return c ? FORWARD : REVERSE;
}

bool stoppedEverywhere(const char *when) {
  bool ok = true;
  for (int i = 0; i < WHEEL_COUNT; i++) {
    const WheelPins &w = WHEEL_PINS[i];
    Drive seen[6];
    int n = 0;
    if (MOTOR_DRIVER == DRIVER_PH_EN) {
      seen[n++] = drv8876(w, 0);
      seen[n++] = drv8876(w, 1);
      seen[n++] = drv8876(w, 2);
    } else if (MOTOR_DRIVER == DRIVER_IN1_IN2) {
      seen[n++] = drv8871(w);
    } else {
      seen[n++] = tb6612(w);
      seen[n++] = l298n(w, false);
      seen[n++] = l298n(w, true);
    }
    for (int k = 0; k < n; k++) {
      if (seen[k] == FORWARD || seen[k] == REVERSE) {
        printf("      %s: %s case %d drives %s\n", when, w.name, k, NAMES[seen[k]]);
        ok = false;
      }
    }
  }
  return ok;
}

Drive mainCase(int i) {
  const WheelPins &w = WHEEL_PINS[i];
  if (MOTOR_DRIVER == DRIVER_PH_EN) return drv8876(w, 0);
  if (MOTOR_DRIVER == DRIVER_IN1_IN2) return drv8871(w);
  return tb6612(w);
}

int main() {
  const char *kind = MOTOR_DRIVER == DRIVER_PH_EN     ? "DRIVER_PH_EN"
                     : MOTOR_DRIVER == DRIVER_IN1_IN2 ? "DRIVER_IN1_IN2"
                                                      : "DRIVER_PWM_DIR";
  printf("motor driver: %s\n", kind);
  setupMotors();
  CHECK(stoppedEverywhere("after setup"), "after setup every motor is stopped, even with the driver wired wrong");

  for (int i = 0; i < WHEEL_COUNT; i++) setWheelTarget(i, 0.5);
  for (int t = 0; t < 50; t++) updateMotors();
  bool fwd = true;
  for (int i = 0; i < WHEEL_COUNT; i++) fwd = fwd && mainCase(i) == FORWARD;
  CHECK(fwd, "+50% drives every motor forward");
  if (MOTOR_DRIVER == DRIVER_PWM_DIR) {
    bool l298 = true;
    for (int i = 0; i < WHEEL_COUNT; i++) l298 = l298 && l298n(WHEEL_PINS[i], false) == FORWARD;
    CHECK(l298, "...on an L298N too");
  }

  stopMotorsNow();
  CHECK(stoppedEverywhere("after stopMotorsNow"), "a stop from +50% leaves every motor stopped");

  for (int i = 0; i < WHEEL_COUNT; i++) setWheelTarget(i, -0.5);
  for (int t = 0; t < 50; t++) updateMotors();
  bool rev = true;
  for (int i = 0; i < WHEEL_COUNT; i++) rev = rev && mainCase(i) == REVERSE;
  CHECK(rev, "-50% drives every motor backward");

  for (int i = 0; i < WHEEL_COUNT; i++) setWheelTarget(i, 0);
  for (int t = 0; t < 50; t++) updateMotors();
  CHECK(stoppedEverywhere("after ramping down"), "ramping down to 0 leaves every motor stopped");
  CHECK(!motorsBusy(), "and the motors count as stopped");

  printf("\n%s: %d failure(s)\n", failures ? "FAILED" : "ALL PASSED", failures);
  return failures != 0;
}
