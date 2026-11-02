// =====================================================================================
//  config.h - the settings of the program
// =====================================================================================
//
//  This file holds the settings that normally change: the board, whether the phone
//  joystick is on, the motor drivers (on other boards), the wheel positions, the speeds
//  and the robot's WiFi name. The other files normally need no changes.
//
//  After changing a setting, upload the program again.
//
// =====================================================================================

#pragma once

#include <Arduino.h>  // so pin names like PA0 (STM32 boards) are known below

// -------------------------------------------------------------------------------------
//  1. Your board
// -------------------------------------------------------------------------------------
//  Leave exactly ONE line without the // in front. The pins of the listed boards are in
//  boards.h. For any other board, select BOARD_MY_OWN and enter its pins further down,
//  under "Your own board's pins". docs/08_Other_Boards.md explains the wiring for each one.
//  (PlatformIO: the section being built selects the board, for example -e arduino_mega,
//  and these lines are skipped. For your own pins, build the my_own section.)

#if !defined(BOARD_ROMEO_ESP32S3) && !defined(BOARD_ESP32_DEVKIT) && \
    !defined(BOARD_ESP32S3_DEVKIT) && !defined(BOARD_ARDUINO_MEGA) && \
    !defined(BOARD_UNO_R4) && !defined(BOARD_STM32_NUCLEO) && !defined(BOARD_MY_OWN)

#define BOARD_ROMEO_ESP32S3     // DFRobot Romeo ESP32-S3: the board of the robot as built
// #define BOARD_ESP32_DEVKIT   // a plain ESP32 board plus separate motor drivers
// #define BOARD_ESP32S3_DEVKIT // an ESP32-S3 DevKitC-1 plus separate motor drivers
// #define BOARD_ARDUINO_MEGA   // Arduino Mega 2560 plus separate motor drivers
// #define BOARD_UNO_R4         // Arduino UNO R4 Minima or WiFi plus separate motor drivers
// #define BOARD_STM32_NUCLEO   // STM32 Nucleo-F401RE plus separate motor drivers
// #define BOARD_MY_OWN         // your own board: fill in "Your own board's pins" below
//
// The classic Arduino Uno (R3) has too little memory for this program: it has 2 KB, and
// the program needs about 2.6 KB. The lessons in firmware/lessons/ work on it.

#endif

// ---- The phone joystick: 1 = on, 0 = off --------------------------------------------
//  On: the robot creates its own WiFi network and serves the driving page to a phone. This
//  requires an ESP32 board; other boards leave it out automatically. Off: no WiFi at all,
//  and a much smaller program. In both cases the Serial Monitor can drive the robot, and
//  it always takes priority.
#ifndef REMOTE_CONTROL
#define REMOTE_CONTROL 1
#endif

// ---- Boards other than the Romeo: motor drivers, and your own board's pins ------------
//  For the Romeo, skip to section 2. Its drivers are built in, and its pins are in boards.h.
//
//  MOTOR_DRIVER states what the pins of the motor driver boards do. Every board in boards.h,
//  and the example under "Your own board's pins" below, is wired for one Pololu DRV8876
//  board per motor: DRIVER_PH_EN. For a different driver, change this line to its kind:
//    DRIVER_PH_EN    one speed pin + one direction pin: speed -> EN (or PWM),
//                    direction -> PH (or DIR). Pololu DRV8876 or DRV8874 boards,
//                    Cytron MD10C / MDD10A
//    DRIVER_IN1_IN2  two pins that both carry speed: speed -> IN1, direction -> IN2.
//                    Both pins must be able to do PWM. DRV8871, BTS7960 (R_EN and L_EN
//                    tied high)
//    DRIVER_PWM_DIR  one speed pin + two direction pins: speed -> PWM (or ENA),
//                    direction -> IN1, direction 2 -> IN2. TB6612FNG (small motors only),
//                    L298N (with its ENA and ENB jumpers taken off)
//  docs/08_Other_Boards.md gives the wiring for each, and lists the drivers that cannot
//  take 12 V.
#ifndef MOTOR_DRIVER
#define MOTOR_DRIVER DRIVER_PH_EN
#endif

#include "boards.h"  // the listed boards, and the names used in the table below

//  Your own board's pins (used only with BOARD_MY_OWN):
//  One line per wheel, in the same order as WHEEL_SETUP in section 2. Enter the pin
//  numbers as wired; -1 means "not used". The example is an ESP32-S3 DevKitC-1 with three
//  DRV8876 driver boards (the same pins as the BOARD_ESP32S3_DEVKIT preset in boards.h).
//    speed, direction  every driver needs these two
//    direction 2       only DRIVER_PWM_DIR uses it (IN2); -1 for the others
//  USE_ENCODERS is 1 if all six encoder pins can use interrupts, and 0 if not, or if there
//  are no encoders (then enter -1 for encoder A and B). A pin left at -1 where one is
//  needed stops the build with a message.

#if defined(BOARD_MY_OWN)

#define BOARD_LABEL "My own board"
#define USE_ENCODERS 1

constexpr WheelPins WHEEL_PINS[3] = {
  // name   speed   direction   direction 2   encoder A   encoder B
  { "W1",     12,       13,         -1,            4,          5 },  // front left (60)
  { "W2",      9,       10,         -1,            6,          7 },  // back (180)
  { "W3",     14,       11,         -1,           15,         16 },  // front right (300)
};

#endif

// -------------------------------------------------------------------------------------
//  2. Your wheels
// -------------------------------------------------------------------------------------
//  One line per wheel, in the same order as the pins in boards.h (or in the table above).
//
//    angle         where the wheel sits around the robot, in degrees, seen from above,
//                  going anticlockwise from the front:
//                  0 = front, 90 = left, 180 = back, 270 = right
//    flipMotor     true if this wheel turns the wrong way (docs/05_First_Drive.md shows
//                  how to check)
//    flipEncoder   true if this wheel's encoder counts backwards (the `test` command
//                  reports this)

struct WheelSetup {
  float angle;
  bool flipMotor;
  bool flipEncoder;
};

// On the robot as built, all three encoders count backwards relative to their motors (as
// reported by the `test` command), so flipEncoder is true. Another build may differ: run
// `test` to check.
const WheelSetup WHEEL_SETUP[3] = {
  // angle   flipMotor   flipEncoder
  {    60,   false,      true },   // front left
  {   180,   false,      true },   // back
  {   300,   false,      true },   // front right
};

// -------------------------------------------------------------------------------------
//  3. Motors and encoders
// -------------------------------------------------------------------------------------

// Encoder counts for one full turn of the wheel. The DFRobot FIT0493 motors give
// 11 pulses per motor turn, the gearbox is 34:1, and the program counts 4 edges per pulse:
// 11 x 34 x 4 = 1496. For different motors, change this number.
const float COUNTS_PER_WHEEL_TURN = 1496;

// The rate at which the motors are switched on and off to set their speed (PWM).
// 20000 times a second is too fast to hear. The Arduino Mega and the UNO R4 ignore this
// setting: they stay at about 490 Hz, which produces a faint whine.
const long MOTOR_PWM_HZ = 20000;

// -------------------------------------------------------------------------------------
//  4. Driving
// -------------------------------------------------------------------------------------

const int START_SPEED = 30;          // % used when a move is typed without a speed
const int START_MOVE_TIME_MS = 1000;  // how long a typed move lasts (0 = until stopped)
const float SPEED_UP_TIME_S = 0.25;   // seconds to go from stopped to full speed (and back)
const int TEST_SPEED = 25;           // % used by the `test` command

// -------------------------------------------------------------------------------------
//  5. Phone joystick (only used when REMOTE_CONTROL is 1, on an ESP32 board)
// -------------------------------------------------------------------------------------

// The robot's own WiFi network (its hotspot) and its web address on a home network:
// http://<WEB_NAME>.local
#define HOTSPOT_NAME "OmniBots_Mini"
#define WEB_NAME "omnibots-mini"

const int PHONE_START_LIMIT = 50;   // % speed limit until the phone sends its own
const int PHONE_TURN_POWER = 50;    // % motor power for a full turn at a 100% limit
const float PHONE_DEADZONE = 0.08;  // knob travel from the centre that is ignored (0..1)

// -------------------------------------------------------------------------------------
//  6. Safety timings: change these only for a specific reason
// -------------------------------------------------------------------------------------

const unsigned long PHONE_SILENCE_STOP_MS = 500;   // no drive message for this long: stop
const unsigned long PHONE_OLD_MESSAGE_MS = 300;    // ignore a phone message this old
const unsigned long PHONE_GONE_MS = 6000;          // no message for this long: release control
const unsigned long WIFI_JOIN_TIMEOUT_MS = 30000;  // home WiFi not joined by now: use the hotspot
