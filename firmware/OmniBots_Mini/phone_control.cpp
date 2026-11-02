// =====================================================================================
//  phone_control.cpp - the robot's handling of the phone's messages (see phone_control.h)
// =====================================================================================
//
//  The messages are short pieces of JSON text, for example:
//
//    {"type":"drive","x":0.0,"y":0.8,"r":0,"spin":0,"limit":50}
//      x, y   where the knob is: right and forward, from -1 to 1
//      r      turn held on the keyboard (Q/E): +1 left, -1 right
//      spin   the Spin buttons: 1 left, 0 off, -1 right
//      limit  the speed limit slider, 10 to 100 %
//    {"type":"enable_joy"}  {"type":"disable_joy"}  {"type":"estop"}  {"type":"ping"}
//
//  Because every "drive" message carries the whole state, a lost message has no lasting
//  effect: the next one, 50 to 100 ms later, replaces it.
//
// =====================================================================================

#include <Arduino.h>

#include "config.h"
#include "phone_control.h"

#if USE_PHONE_JOYSTICK

#include "drive.h"
#include "serial_commands.h"

struct PhoneState {
  uint32_t client;     // the page in control, 0 = none
  bool joystickOn;     // its Joystick mode switch is on: it drives the wheels
  float x, y, r;       // knob right/forward (-1..1), held turn (+ = left)
  int spin;            // Spin buttons: 1 left, -1 right, 0 off
  int limit;           // speed limit in %
  uint32_t heardMs;    // when anything was last received from it
  uint32_t driveMs;    // when its last "drive" message arrived (the half-second stop)
  float angle, speed, turn;  // the present motion, for the page's readouts
};

PhoneState phone = { 0, false, 0, 0, 0, 0, PHONE_START_LIMIT, 0, 0, 0, 0, 0 };

void clearPhoneInput() {
  phone.x = 0;
  phone.y = 0;
  phone.r = 0;
  phone.spin = 0;
}

void phoneStandDown() {
  if (!phone.joystickOn) return;
  phone.joystickOn = false;
  clearPhoneInput();
  Serial.println("   web Joystick mode turned off");
}

void updatePhoneDriving(uint32_t now) {
  if (!phone.joystickOn) {
    phone.speed = 0;
    phone.angle = 0;
    phone.turn = 0;
    return;
  }

  // The safety stop: with no "drive" message for this long, nobody is driving any more.
  if (now - phone.driveMs > PHONE_SILENCE_STOP_MS) clearPhoneInput();

  // How far the knob is pushed, ignoring small movements around the centre.
  float pushed = sqrt(phone.x * phone.x + phone.y * phone.y);
  if (pushed > 1) pushed = 1;
  float share = 0;
  if (pushed > PHONE_DEADZONE) share = (pushed - PHONE_DEADZONE) / (1 - PHONE_DEADZONE);

  float limit = phone.limit / 100.0;
  float forward = 0;
  float left = 0;
  if (share > 0) {
    forward = phone.y / pushed * share * limit;  // knob up = forward
    left = -phone.x / pushed * share * limit;    // knob right = go right
  }
  float turn = phone.spin + phone.r;
  if (turn > 1) turn = 1;
  if (turn < -1) turn = -1;
  float turnPower = turn * (PHONE_TURN_POWER / 100.0) * limit;

  driveRobot(forward, left, turnPower);

  // Store the present motion for the page.
  phone.speed = share * limit * 100;
  phone.turn = turnPower * 100;
  // Direction counts anticlockwise from the front, as everywhere else in the
  // program (go 90 = left): knob left = 90, knob right = 270.
  phone.angle = 0;
  if (share > 0) phone.angle = atan2(-phone.x, phone.y) * RAD_TO_DEG;
  if (phone.angle < 0) phone.angle += 360;
}

// ---- Reading the JSON messages ----
// The messages are always flat and simple, so a small reader is sufficient.

// Where the value of "key": starts, or nullptr.
const char *findValue(const char *json, const char *key) {
  char pattern[24];
  snprintf(pattern, sizeof pattern, "\"%s\"", key);
  const char *p = strstr(json, pattern);
  if (!p) return nullptr;
  p += strlen(pattern);
  while (*p == ' ') p++;
  if (*p != ':') return nullptr;
  p++;
  while (*p == ' ') p++;
  return p;
}

bool readNumber(const char *json, const char *key, float &value) {
  const char *p = findValue(json, key);
  if (!p) return false;
  char *end;
  value = strtod(p, &end);
  return end != p && fabs(value) < 1e9;  // also rules out "nan" and "inf"
}

bool readText(const char *json, const char *key, char *text, size_t size) {
  const char *p = findValue(json, key);
  if (!p || *p != '"') return false;
  p++;
  size_t n = 0;
  while (*p && *p != '"' && n + 1 < size) text[n++] = *p++;
  text[n] = 0;
  return *p == '"';
}

// ---- Pages connecting and disconnecting ----

bool phoneConnected(uint32_t client, uint32_t now) {
  if (phone.client) return false;  // another page is already in control
  phone.client = client;
  phone.joystickOn = false;
  clearPhoneInput();
  phone.limit = PHONE_START_LIMIT;
  phone.heardMs = now;
  if (someoneTyped()) Serial.println("   web: a page took control");
  return true;
}

void phoneDisconnected(uint32_t client) {
  if (client != phone.client) return;
  phone.client = 0;
  if (phone.joystickOn) {
    phone.joystickOn = false;
    clearPhoneInput();
    stopRobot();
  }
  if (someoneTyped()) Serial.println("   web: the page let go");
}

uint32_t phoneInControl() {
  return phone.client;
}

uint32_t phoneGoneQuiet(uint32_t now) {
  if (!phone.client || now - phone.heardMs < PHONE_GONE_MS) return 0;
  uint32_t client = phone.client;
  phoneDisconnected(client);
  return client;
}

void phoneMessagesLost() {
  clearPhoneInput();
}

// ---- One message ----

const char *phoneMessage(uint32_t client, const char *message, uint32_t receivedMs, uint32_t now) {
  if (client != phone.client) return nullptr;  // not the page in control: ignore it
  phone.heardMs = receivedMs;

  char type[16];
  if (!readText(message, "type", type, sizeof type)) return nullptr;

  if (strcmp(type, "ping") == 0) return "{\"type\":\"pong\"}";

  if (strcmp(type, "drive") == 0) {
    // Skip the message if Joystick mode is off, if it waited too long on arrival, or if any
    // part is missing or invalid. The next message follows shortly.
    float x, y, r, spin, limit;
    if (!phone.joystickOn || now - receivedMs > PHONE_OLD_MESSAGE_MS) return nullptr;
    if (!readNumber(message, "x", x) || !readNumber(message, "y", y) ||
        !readNumber(message, "r", r) || !readNumber(message, "spin", spin) ||
        !readNumber(message, "limit", limit)) {
      return nullptr;
    }
    if (spin != 1 && spin != 0 && spin != -1) return nullptr;

    float pushed = sqrt(x * x + y * y);
    if (pushed > 1) {  // keep the knob inside its circle
      x = x / pushed;
      y = y / pushed;
    }
    phone.x = x;
    phone.y = y;
    phone.r = constrain(r, -1.0f, 1.0f);
    phone.spin = (int)spin;
    phone.limit = (int)lround(constrain(limit, 10.0f, 100.0f));
    phone.driveMs = receivedMs;
  } else if (strcmp(type, "enable_joy") == 0) {
    if (!phone.joystickOn) {
      stopRobot();        // the phone takes over from any move typed in the Serial Monitor
      cancelTimedMove();
      clearPhoneInput();
      phone.joystickOn = true;
      if (someoneTyped()) Serial.println("   web: Joystick mode on");
    }
  } else if (strcmp(type, "disable_joy") == 0) {
    if (phone.joystickOn) {
      phone.joystickOn = false;
      clearPhoneInput();
      stopRobot();
      if (someoneTyped()) Serial.println("   web: Joystick mode off");
    }
  } else if (strcmp(type, "estop") == 0) {
    clearPhoneInput();
    stopRobot();
  }
  return nullptr;  // any other message only shows that the page is still connected
}

size_t phoneStatus(char *text, size_t size, bool hotspot, int rssi) {
  char rssiText[8] = "null";  // no signal reading on the hotspot
  if (rssi) snprintf(rssiText, sizeof rssiText, "%d", rssi);
  return snprintf(text, size,
                  "{\"type\":\"telemetry\",\"joy_enabled\":%s,\"spin\":%d,\"angle\":%d,"
                  "\"speed\":%d,\"turn\":%d,\"limit\":%d,\"net\":\"%s\",\"rssi\":%s}",
                  phone.joystickOn ? "true" : "false", phone.spin, (int)lround(phone.angle) % 360,
                  (int)lround(phone.speed), (int)lround(phone.turn), phone.limit,
                  hotspot ? "ap" : "sta", rssiText);
}

#else  // ---- phone joystick off: empty stand-ins, so the rest of the program is unchanged ----

void updatePhoneDriving(uint32_t) {}
void phoneStandDown() {}

#endif
