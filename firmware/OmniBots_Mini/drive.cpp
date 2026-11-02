// =====================================================================================
//  drive.cpp - the omni mathematics (drive.h defines the numbers)
// =====================================================================================

#include <Arduino.h>

#include "config.h"
#include "drive.h"
#include "motors.h"

void driveRobot(float forward, float left, float turn) {
  float wheel[WHEEL_COUNT];

  // 1. How much each wheel has to roll to move the robot in that direction.
  //    A wheel can push only along its own rolling direction, which is at
  //    right angles to the line from the robot's centre to the wheel. A wheel
  //    at angle a pushes the robot along (-sin a, cos a).
  float biggest = 0;
  for (int i = 0; i < WHEEL_COUNT; i++) {
    float a = WHEEL_SETUP[i].angle * DEG_TO_RAD;
    wheel[i] = -sin(a) * forward + cos(a) * left;
    if (fabs(wheel[i]) > biggest) biggest = fabs(wheel[i]);
  }

  // 2. Scale the result so that the busiest wheel gets exactly the requested share of
  //    top speed.
  float speed = sqrt(forward * forward + left * left);
  float scale = 0;
  if (biggest > 0.000001) scale = speed / biggest;

  // 3. Add the turn: to turn, every wheel pushes in the same direction around the robot.
  biggest = 0;
  for (int i = 0; i < WHEEL_COUNT; i++) {
    wheel[i] = wheel[i] * scale + turn;
    if (fabs(wheel[i]) > biggest) biggest = fabs(wheel[i]);
  }

  // 4. Driving and turning together can ask a wheel for more than 100%.
  //    In that case every wheel slows by the same factor, so the robot keeps its
  //    direction at a lower speed.
  float shrink = 1;
  if (biggest > 1) shrink = 1 / biggest;
  for (int i = 0; i < WHEEL_COUNT; i++) setWheelTarget(i, wheel[i] * shrink);
}

void stopRobot() {
  stopMotorsNow();
}
