// =====================================================================================
//  OmniBots_Mini.ino - the full program for Mini Omni Bot, a 3-wheel omni robot
// =====================================================================================
//
//  Functions of the program:
//    - driving by commands typed in the Serial Monitor, over the USB cable (115200 baud,
//      line ending "New Line"; type help for the list)
//    - driving from a phone over WiFi with the phone joystick (ESP32 boards, on unless
//      REMOTE_CONTROL is 0 in config.h; type wifi for the connection details)
//    - measuring how far each wheel turns with the motor encoders
//
//  Complete the lessons in firmware/lessons/ first: they build up to this program.
//  docs/05_First_Drive.md then explains how to upload and drive it.
//
//  File overview:
//    config.h             the settings: board, wheel layout, speeds, WiFi name
//    boards.h             the pin assignments for each board
//    motors.cpp           motor output, with gradual changes of speed
//    encoders.cpp         counting wheel turns
//    drive.cpp            the omni mathematics: a direction becomes a power for each wheel
//    serial_commands.cpp  the commands typed in the Serial Monitor
//  The phone joystick (the lines marked [phone joystick] below connect it; when it is
//  off, they do nothing):
//    phone_control.cpp    the handling of the phone's messages, and the safety stops
//    wifi_server.cpp      WiFi and the web server (ESP32 only)
//    web_assets.h         the driving page, packed into the program
//
// =====================================================================================

#include "config.h"
#include "drive.h"
#include "encoders.h"
#include "motors.h"
#include "phone_control.h"
#include "serial_commands.h"
#include "wifi_server.h"

unsigned long lastTickMs = 0;

void setup() {
  Serial.begin(115200);
  setupMotors();    // every wheel starts stopped
  setupEncoders();  // start counting wheel turns
  setupWifi();      // [phone joystick] the hotspot (or home WiFi) and the driving page
  // The robot prints nothing until something is typed. Type help for the command list.
}

void loop() {
  readSerialCommands();  // handle text typed in the Serial Monitor
  updateWifi();          // [phone joystick] handle messages from the phone

  // Everything below runs 100 times a second.
  unsigned long now = millis();
  if (now - lastTickMs >= 10) {
    lastTickMs = now;
    updatePhoneDriving(now);  // [phone joystick] knob to wheel targets, and the safety stop
    updateTimedMove(now);     // end a typed move when its time is up
    updateMotors();           // move each wheel's power towards its target
    updateWheelSpeeds(now);   // calculate each wheel's rpm
  }
}
