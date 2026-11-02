// =====================================================================================
//  boards.h - the pin assignments for each listed board
// =====================================================================================
//
//  This file normally needs no changes. Select the board in config.h and the matching
//  block below is used. The kind of motor driver (MOTOR_DRIVER) is also set in config.h.
//
//  For a board that is not listed, select BOARD_MY_OWN in config.h and enter its pins in
//  the table there. docs/08_Other_Boards.md explains the procedure.
//
//  The classic Arduino Uno (R3) is not listed: it has only 2 KB of memory and the full
//  program needs about 2.6 KB. The lessons in firmware/lessons/ do run on it. The UNO R4
//  has a preset below.
//
//  Every block states how far it has been tried:
//    Tested         driven on a real robot (so far only the Romeo)
//    Builds         compile-checked only, not yet driven on a real robot
//    User-reported  driven by a user on their own robot, and reported (none yet)
//
//  Each block also checks that the Arduino IDE (Tools > Board) or platformio.ini is set to
//  a matching board, and stops the build with a message if not. The check cannot tell every
//  pair of boards apart, so confirm the board name that `help` prints on its last line.
//
//  Every block sets three things:
//
//    WHEEL_PINS     one line per wheel:
//                     name   - what the wheel is called in commands and messages
//                     pin1   - DRIVER_PH_EN: EN    DRIVER_IN1_IN2: IN1   DRIVER_PWM_DIR: PWM
//                     pin2   - DRIVER_PH_EN: PH    DRIVER_IN1_IN2: IN2   DRIVER_PWM_DIR: IN1
//                     pin3   - DRIVER_PWM_DIR: IN2. The other kinds do not use it.
//                     encA   - encoder channel A (-1 if there is no encoder)
//                     encB   - encoder channel B (-1 if there is no encoder)
//
//    USE_ENCODERS   1 if all six encoder pins can trigger interrupts on this board,
//                   0 if not (the robot still drives, but cannot count wheel turns)
//
//    BOARD_LABEL    a short name for the board, shown by the help command
//
//  The three kinds of motor driver (MOTOR_DRIVER, set in config.h):
//    DRIVER_PH_EN    one speed pin (EN) + one direction pin (PH)
//                    DRV8876 (on the Romeo, and Pololu's DRV8876 and DRV8874 boards),
//                    Cytron MD10C / MDD10A ("PWM + DIR")
//    DRIVER_IN1_IN2  two pins that both carry speed (PWM): pulse IN1 to go one way, IN2 to
//                    go the other
//                    DRV8871, BTS7960 (R_EN, L_EN high), DRV8833, DRV8835 (MODE low), L9110S
//    DRIVER_PWM_DIR  one speed pin (PWM) + two direction pins (IN1, IN2)
//                    TB6612FNG, L298N (with its ENA and ENB jumpers taken off)
//
//  Choosing a driver for the FIT0493 motor: on 12 V it draws about 1.65 A at its
//  maximum-power point and 5.5 A when stalled. The presets below use three Pololu DRV8876
//  boards (#4036), one per motor: the Romeo's own chip, in the same PH/EN mode, with a
//  current limit that protects it. Pololu's DRV8874 board (#4035) has the same pins and
//  takes more current. A DRV8871 board per motor (DRIVER_IN1_IN2) is the other good choice.
//  Every preset also keeps a pin3, so a TB6612FNG or an L298N (DRIVER_PWM_DIR) needs no new
//  pins. These limits apply:
//    - TB6612FNG: only for small motors, about 1 A, and it has no current limit. It is too
//      small for the FIT0493.
//    - DRV8833, DRV8835 and L9110S: cannot take a 12 V battery.
//    - L298N: loses about 2 V internally, and becomes hot.
//  docs/08_Other_Boards.md gives the wiring and the reasons.
//
// =====================================================================================

#pragma once

#define DRIVER_PH_EN 1
#define DRIVER_IN1_IN2 2
#define DRIVER_PWM_DIR 3

struct WheelPins {
  const char *name;
  int pin1, pin2, pin3;
  int encA, encB;
};

// If two boards are selected (for example, a // left off in config.h), stop here, before
// any other check, instead of silently using the first one.
#if defined(BOARD_ROMEO_ESP32S3) + defined(BOARD_ESP32_DEVKIT) + defined(BOARD_ESP32S3_DEVKIT) + \
    defined(BOARD_ARDUINO_MEGA) + defined(BOARD_UNO_R4) + defined(BOARD_STM32_NUCLEO) + \
    defined(BOARD_MY_OWN) > 1
#error "More than one board picked: leave exactly one board line in config.h without //"
#endif

// -------------------------------------------------------------------------------------
//  DFRobot Romeo ESP32-S3 (DFR0994) - the board the robot is designed around.
//  Status: Tested. It drives the robot as built.
//
//  Motor driver: 4 x DRV8876 on the board. Fit the PMODE jumper (PH/EN mode).
//  Motors go in M1, M3 and M4. M2 stays free: its pins are shared with the display
//  connector. M1 also needs its two small jumpers (M1EN / M1PH) fitted.
//  Encoders: all six wires go on the IO4..IO16 header row, powered from 3V3.
// -------------------------------------------------------------------------------------
#if defined(BOARD_ROMEO_ESP32S3)

#if defined(ARDUINO) && (!defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ARDUINO_ESP32S3_DEV))
#error "config.h picks the Romeo ESP32-S3, but Tools > Board (or platformio.ini) is another \
board: pick yours in config.h, or set Tools > Board to DFRobot Romeo ESP32-S3"
#endif
#if MOTOR_DRIVER != DRIVER_PH_EN
#error "The Romeo's own motor drivers are DRV8876s: set MOTOR_DRIVER in config.h back to \
DRIVER_PH_EN"
#endif

#define BOARD_LABEL "Romeo ESP32-S3"
#define USE_ENCODERS 1

constexpr WheelPins WHEEL_PINS[3] = {
  // name   pin1 (EN)  pin2 (PH)  pin3   encA  encB
  { "M1",      12,        13,       -1,     4,    5 },
  { "M3",       9,        10,       -1,     6,    7 },
  { "M4",      47,        11,       -1,    15,   16 },
};

// -------------------------------------------------------------------------------------
//  ESP32 DevKit: a plain ESP32 board with an ESP32-WROOM-32 module, 30 or 38 pins
//  (sold as "ESP32 DevKit V1" or "ESP32-DevKitC"). Arduino IDE board: "ESP32 Dev Module".
//  Status: Builds. Compile-checked only, not yet driven on a real robot.
//
//  Motor driver: 3 x Pololu DRV8876 boards, one per motor, PMODE to GND and SLEEP to 5V
//  (the pin marked 5V, or VIN on some boards).
//  Other drivers: DRV8871 -> DRIVER_IN1_IN2 (every pin2 here can do PWM too).
//                 TB6612FNG (small motors only) or L298N -> DRIVER_PWM_DIR (pin3 is ready).
//  Pins: none of the boot (strapping) pins 0, 2, 5, 12, 15, none of the flash pins 6-11,
//  and not 1/3 (USB serial). 16/17 stay free: on WROVER modules they belong to the memory.
//  Encoders: power them from 3V3, never 5 V. W3's encoder is on 34 and 35, which are
//  input-only and have no internal pull-up. If `test` finds no pulses on W3 only, add a
//  10 kOhm resistor from 34 to 3V3 and another from 35 to 3V3.
// -------------------------------------------------------------------------------------
#elif defined(BOARD_ESP32_DEVKIT)

#if defined(ARDUINO) && !defined(CONFIG_IDF_TARGET_ESP32)
#error "config.h picks the ESP32 DevKit, but Tools > Board (or platformio.ini) is another \
board: pick yours in config.h, or set Tools > Board to ESP32 Dev Module"
#endif

#define BOARD_LABEL "ESP32 DevKit"
#define USE_ENCODERS 1

constexpr WheelPins WHEEL_PINS[3] = {
  // name   pin1 (EN)  pin2 (PH)  pin3 (only for DRIVER_PWM_DIR)  encA  encB
  { "W1",      25,        26,        27,                           32,   33 },
  { "W2",      23,        22,        21,                           14,   13 },
  { "W3",      19,        18,         4,                           34,   35 },
};

// -------------------------------------------------------------------------------------
//  ESP32-S3 DevKitC-1: Espressif's ESP32-S3 board and its clones (N8, N8R8, N16R8, ...).
//  Arduino IDE board: "ESP32S3 Dev Module".
//  Status: Builds. Compile-checked only, not yet driven on a real robot.
//
//  USB: use the port marked UART (COM on some clones) and leave Tools > "USB CDC On Boot"
//  on Disabled (the default): the Serial Monitor communicates through that port. To use the
//  port marked USB, or on a board whose only port connects directly to the ESP32-S3, set
//  "USB CDC On Boot" to Enabled (PlatformIO: add -DARDUINO_USB_CDC_ON_BOOT=1 to build_flags).
//  Motor driver: 3 x Pololu DRV8876 boards, one per motor, PMODE to GND and SLEEP to 5V.
//  Other drivers: DRV8871 -> DRIVER_IN1_IN2 (every pin2 here can do PWM too).
//                 TB6612FNG (small motors only) or L298N -> DRIVER_PWM_DIR (pin3 is ready).
//  Pins: all on the J1 header (the side with 4 to 18). The motor and encoder pins are the
//  Romeo's own: W1 = M1, W2 = M3, and W3 = M4 with 14 in place of 47. A program built for
//  the other one of the two boards therefore cannot start a wheel by itself, and the lessons
//  run with one change. Not used: the boot (strapping) pins 0, 3, 45 and 46, the USB pins
//  19/20, 26 to 37 (flash and PSRAM), the RGB LED (38 or 48) and 43/44 (the UART port's
//  serial). 18 is used only as pin3, a direction pin: it can pulse high for 60 microseconds
//  at power-up (ESP32-S3 datasheet v2.2, table 2-2).
//  Encoders: power them from 3V3, never 5 V.
// -------------------------------------------------------------------------------------
#elif defined(BOARD_ESP32S3_DEVKIT)

#if defined(ARDUINO) && \
    (!defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ARDUINO_DFROBOT_ROMEO_ESP32S3))
#error "config.h picks the ESP32-S3 DevKitC-1, but Tools > Board (or platformio.ini) is another \
board: pick yours in config.h, or set Tools > Board to ESP32S3 Dev Module"
#endif

#define BOARD_LABEL "ESP32-S3 DevKitC-1"
#define USE_ENCODERS 1

constexpr WheelPins WHEEL_PINS[3] = {
  // name   pin1 (EN)  pin2 (PH)  pin3 (only for DRIVER_PWM_DIR)  encA  encB
  { "W1",      12,        13,        17,                            4,    5 },
  { "W2",       9,        10,        18,                            6,    7 },
  { "W3",      14,        11,         8,                           15,   16 },
};

// -------------------------------------------------------------------------------------
//  Arduino Mega 2560 (5 V logic, no WiFi: drive it from the Serial Monitor)
//  Status: Builds. Compile-checked only, not yet driven on a real robot.
//
//  Motor driver: 3 x Pololu DRV8876 boards, one per motor, PMODE to GND and SLEEP to 5V.
//  Other drivers: TB6612FNG (small motors only) or L298N -> DRIVER_PWM_DIR (pin3 is
//  ready). DRIVER_IN1_IN2 would need three more PWM pins: 22, 24 and 26 cannot produce PWM.
//  Power the encoders from 5V.
//  Speed pins 6, 7, 8 share one timer, so all three motors get the same PWM (about
//  490 Hz, which produces a faint whine from the motors).
//  Encoders: the Mega has exactly six interrupt pins (2, 3, 18, 19, 20, 21) and the
//  encoders need all six. Serial1 (18/19) and I2C (20/21) are therefore not available for
//  anything else.
//  Pins 0/1 (USB serial) and 13 (the on-board LED) stay free.
// -------------------------------------------------------------------------------------
#elif defined(BOARD_ARDUINO_MEGA)

#if defined(ARDUINO) && !defined(__AVR_ATmega2560__)
#error "config.h picks the Arduino Mega, but Tools > Board (or platformio.ini) is another \
board: pick yours in config.h, or set Tools > Board to Arduino Mega or Mega 2560"
#endif
#if MOTOR_DRIVER == DRIVER_IN1_IN2
#error "DRIVER_IN1_IN2 needs PWM on both pins, and the Mega preset's direction pins can't do \
it: use BOARD_MY_OWN with PWM pins (docs/08_Other_Boards.md)"
#endif

#define BOARD_LABEL "Arduino Mega"
#define USE_ENCODERS 1

constexpr WheelPins WHEEL_PINS[3] = {
  // name   pin1 (EN)  pin2 (PH)  pin3 (only for DRIVER_PWM_DIR)  encA  encB
  { "W1",       6,        22,        23,                            2,    3 },
  { "W2",       7,        24,        25,                           18,   19 },
  { "W3",       8,        26,        27,                           20,   21 },
};

// -------------------------------------------------------------------------------------
//  Arduino UNO R4 Minima and UNO R4 WiFi: one preset for both. 5 V logic, no phone
//  joystick: drive it from the Serial Monitor. Arduino IDE boards: "Arduino UNO R4
//  Minima" or "Arduino UNO R4 WiFi", from the "Arduino UNO R4 Boards" package.
//  (The program runs on the R4's own chip. The ESP32-S3 on the R4 WiFi is only its WiFi
//  modem and USB link: never upload another program to it. Arduino's firmware updater
//  is safe to use.)
//  Status: Builds. Compile-checked only, not yet driven on a real robot.
//
//  Motor driver: 3 x Pololu DRV8876 boards, one per motor, PMODE to GND and SLEEP to 5V.
//  Other drivers: TB6612FNG (small motors only) or L298N -> DRIVER_PWM_DIR (pin3 is
//  ready). DRIVER_IN1_IN2 would need other pins: 4 and 7 are not PWM pins.
//  Encoders: power them from 5V. At 5 V the R4 requires 4 V before it reads a "high".
//  On an R4, interrupts belong to channels, not pins: a second pin on the same channel
//  silently replaces the first, and a pin with no channel silently does nothing. These six
//  pins are on six different channels on both boards (the two boards connect some pins
//  differently): Minima 0, 9, 3, 6, 7, 2 and WiFi 1, 9, 5, 6, 7, 2.
//  Speed pins 3, 5 and 6 run at the R4's usual PWM, about 490 Hz, with a faint whine like
//  the Mega. (Silent 20 kHz on the R4 needs different code, which is not implemented.)
//  Kept free: 0/1 (Serial1), 13 (the LED) and A4/A5 (I2C). 10, 11 and 12 are also the SPI
//  pins, so SPI cannot be used with this wiring.
// -------------------------------------------------------------------------------------
#elif defined(BOARD_UNO_R4)

#if defined(ARDUINO) && !defined(ARDUINO_UNOR4_MINIMA) && !defined(ARDUINO_UNOR4_WIFI)
#error "config.h picks the UNO R4, but Tools > Board (or platformio.ini) is another board: pick \
yours in config.h, or set Tools > Board to Arduino UNO R4 Minima or WiFi"
#endif
#if MOTOR_DRIVER == DRIVER_IN1_IN2
#error "DRIVER_IN1_IN2 needs PWM on both pins, and the UNO R4 preset's direction pins can't do \
it: use BOARD_MY_OWN with PWM pins (docs/08_Other_Boards.md)"
#endif

#define BOARD_LABEL "Arduino UNO R4"
#define USE_ENCODERS 1

constexpr WheelPins WHEEL_PINS[3] = {
  // name   pin1 (EN)  pin2 (PH)  pin3 (only for DRIVER_PWM_DIR)  encA  encB
  { "W1",       3,         4,        10,                            2,    8 },
  { "W2",       5,         7,        11,                           12,   A1 },
  { "W3",       6,         9,        A0,                           A2,   A3 },
};

// -------------------------------------------------------------------------------------
//  STM32 Nucleo-F401RE with the STM32duino core ("STM32 MCU based boards").
//  3.3 V logic, no WiFi: drive it from the Serial Monitor.
//  Status: Builds. Compile-checked only, not yet driven on a real robot.
//
//  Motor driver: 3 x Pololu DRV8876 boards, one per motor, PMODE to GND and SLEEP to 5V.
//  Other drivers: TB6612FNG (small motors only) or L298N -> DRIVER_PWM_DIR (pin3 is
//  ready). DRIVER_IN1_IN2 would need other pins: PA4 and PC1 cannot produce PWM.
//  Power the encoders from 3V3.
//  Pin names are the chip's own (PA10); the label on the Arduino-style header (D2) is in
//  the comment. All the pins are on those headers.
//  Encoders: on an STM32, pins with the same number share one interrupt (PA8 and PB8
//  cannot both have one), so the six encoder pins have six different numbers.
//  Speed pins: each is on its own timer channel, so the three PWM signals do not conflict.
//  Kept free: PA2/PA3 (D1/D0, the USB serial through the ST-Link), PA5 (D13, the green
//  LED), PC13 (the blue button) and PB8/PB9 (D15/D14, I2C).
// -------------------------------------------------------------------------------------
#elif defined(BOARD_STM32_NUCLEO)

#if defined(ARDUINO) && !defined(ARDUINO_ARCH_STM32)
#error "config.h picks the Nucleo-F401RE, but Tools > Board (or platformio.ini) is another \
board: pick yours in config.h, or set Tools > Board to Nucleo-64"
#endif
#if MOTOR_DRIVER == DRIVER_IN1_IN2
#error "DRIVER_IN1_IN2 needs PWM on both pins, and the Nucleo preset's direction pins can't do \
it: use BOARD_MY_OWN with PWM pins (docs/08_Other_Boards.md)"
#endif

#define BOARD_LABEL "Nucleo-F401RE"
#define USE_ENCODERS 1

constexpr WheelPins WHEEL_PINS[3] = {
  // name   pin1 (EN)  pin2 (PH)  pin3 (PWM_DIR)  encA  encB     header labels
  { "W1",     PB10,       PA0,        PA1,       PA10,  PB3 },  // D6,  A0, A1,  D2, D3
  { "W2",     PC7,        PA4,        PB0,       PB5,   PB4 },  // D9,  A2, A3,  D4, D5
  { "W3",     PB6,        PC1,        PC0,       PA8,   PA9 },  // D10, A4, A5,  D7, D8
};

// -------------------------------------------------------------------------------------
//  Your own board: its pins are in config.h, under "Your own board's pins".
// -------------------------------------------------------------------------------------
#elif defined(BOARD_MY_OWN)

#else
#error "No board picked: choose one in config.h"
#endif

// -------------------------------------------------------------------------------------
//  The phone joystick runs on the ESP32's own web server, so it is built in only when
//  REMOTE_CONTROL is 1 in config.h AND this is an ESP32 board. The rest of the program
//  tests only USE_PHONE_JOYSTICK.
// -------------------------------------------------------------------------------------
#if REMOTE_CONTROL && defined(ESP32)
#define USE_PHONE_JOYSTICK 1
#else
#define USE_PHONE_JOYSTICK 0
#endif
