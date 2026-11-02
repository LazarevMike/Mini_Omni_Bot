// =====================================================================================
//  motors.h - making the wheels turn
// =====================================================================================
//
//  The program asks each wheel for a power between -1.0 (full reverse) and +1.0 (full
//  forward). motors.cpp converts that into the signals for the motor driver (the top of
//  boards.h explains the three kinds of driver).
//
//  Wheels do not jump to a new speed. They speed up and slow down over SPEED_UP_TIME_S
//  (config.h), which reduces the load on the gearboxes and prevents the robot from lurching.
//
// =====================================================================================

#pragma once

const int WHEEL_COUNT = 3;
const int PWM_MAX = 255;  // analogWrite() goes from 0 (off) to 255 (full power)

// Set up the motor pins. Every wheel starts stopped.
void setupMotors();

// Ask one wheel for a power from -1.0 to +1.0. It reaches that power gradually.
void setWheelTarget(int wheel, float power);

// Stop every wheel immediately, with no gradual slowing. The driver brakes the motors.
void stopMotorsNow();

// Move each wheel's power a small step towards its target. Call every 10 ms.
void updateMotors();

// True while any wheel is turning, or has been asked to.
bool motorsBusy();
