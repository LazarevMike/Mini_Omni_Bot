// =====================================================================================
//  serial_commands.cpp - the commands typed in the Serial Monitor (see serial_commands.h)
// =====================================================================================
//
//  How typed text arrives depends on the program used:
//    - The Arduino IDE's Serial Monitor sends the whole line when Enter is pressed, plus
//      the line ending selected in its drop-down: New Line (\n, the default), Carriage
//      Return (\r) or Both NL & CR (\r\n). All three work here.
//    - PlatformIO's Serial Monitor also sends the whole line on Enter (the send_on_enter
//      filter in platformio.ini).
//    - Other serial programs (PuTTY, screen, ...) send each key as it is pressed.
//  In all cases, a command runs only when its Enter arrives, never as a partial line.
//  With "No Line Ending" no Enter ever arrives. A line that arrived all at once, as the
//  Serial Monitor sends it, and still has no line ending after LINE_WAIT_MS therefore never
//  runs: the robot stops, prints what to change, and refuses that line even if an Enter
//  arrives later.
//
//  The checks are strict for a reason. When a program opens the USB port, it can echo the
//  robot's printed text straight back. If the robot then read "m1 +25%" as a command, a
//  wheel would start turning unasked. For that reason:
//    - numbers must be plain numbers ("25", "-40", "1.5"), nothing else
//    - every line the robot prints starts with spaces or a word that is not a command
//    - any invalid line stops the robot, and the robot prints the reason
//
// =====================================================================================

#include <Arduino.h>
#include <stdarg.h>

#include "config.h"
#include "drive.h"
#include "encoders.h"
#include "motors.h"
#include "phone_control.h"
#include "serial_commands.h"
#include "wifi_server.h"

// ---- The moves that can be typed ----

struct Motion {
  const char *names[4];  // the accepted text (up to 4 spellings)
  const char *label;     // the name the robot prints
  float forward, left, turn;
};

const Motion MOTIONS[] = {
  { { "w", "f", "forward" }, "forward",       1,  0,  0 },
  { { "s", "b", "back" },    "back",         -1,  0,  0 },
  { { "a", "l", "left" },    "slide left",    0,  1,  0 },
  { { "d", "r", "right" },   "slide right",   0, -1,  0 },
  { { "q", "ccw" },          "turn left",     0,  0,  1 },
  { { "e", "cw" },           "turn right",    0,  0, -1 },
};
const int MOTION_COUNT = sizeof(MOTIONS) / sizeof(MOTIONS[0]);

// ---- The state of this file ----

int defaultSpeed = START_SPEED;                 // % for moves typed without a speed
unsigned long defaultTimeMs = START_MOVE_TIME_MS;  // how long those moves last

bool moveActive = false;            // a typed move is running
unsigned long moveStartMs = 0;
unsigned long moveMs = 0;           // 0 = until stopped
long countAtMoveStart[WHEEL_COUNT];  // encoder counts when the move started

char line[128];                     // the text typed so far
int lineLength = 0;
bool lineTypedByHand = false;       // its characters came one by one, with pauses between
bool lineTooLate = false;           // it waited too long for its Enter: it will never run
bool hintGiven = false;             // ...and the robot has printed so since the last character
char previousChar = 0;
unsigned long previousCharMs = 0;   // when the last character arrived
bool typedSomething = false;

// A line that arrived all at once (as the Serial Monitor sends it) and still has no line
// ending after this long never receives one: the line ending is set to "No Line Ending".
const unsigned long LINE_WAIT_MS = 1500;
// A gap longer than this between two characters means that they are being typed one by one,
// in a program that sends each key as it is pressed. That line waits for its Enter without
// a time limit.
const unsigned long KEY_GAP_MS = 100;
// Enter can arrive as two characters, \r then \n, sent together. A \n that arrives later
// than this after a \r is a separate Enter.
const unsigned long CR_LF_GAP_MS = 100;

// ---- Printing ----
// AVR boards such as the Arduino Mega have no Serial.printf(), and their printf cannot print
// decimals, so this file builds its text with these two helpers. They work on every board.

// Like printf, for text and whole numbers.
void say(const char *format, ...) {
  char text[160];
  va_list args;
  va_start(args, format);
  vsnprintf(text, sizeof text, format, args);
  va_end(args);
  Serial.print(text);
}

// A number with a + or - in front, for example +1.25
void sayNumber(float value, int decimals) {
  if (value >= 0) Serial.print('+');
  Serial.print(value, decimals);
}

// ---- Moves ----

void printMoveCounts(const char *how) {
  say("   %-8s", how);
  for (int wheel = 0; wheel < WHEEL_COUNT; wheel++) {
    say("  %s %+ld", WHEEL_PINS[wheel].name, encoderCount(wheel) - countAtMoveStart[wheel]);
  }
  Serial.println(" counts");
}

void startTimedMove(const char *label, int speed, unsigned long ms) {
  for (int wheel = 0; wheel < WHEEL_COUNT; wheel++) countAtMoveStart[wheel] = encoderCount(wheel);
  moveActive = true;
  moveStartMs = millis();
  moveMs = ms;
  say("-> %s %d%% ", label, speed);
  if (ms) say("for %lu ms\r\n", ms);
  else Serial.println("until stopped (press Enter)");
}

void updateTimedMove(unsigned long now) {
  if (!moveActive || moveMs == 0 || now - moveStartMs < moveMs) return;

  // The time has ended: let the wheels slow down gradually and report how far they turned.
  for (int wheel = 0; wheel < WHEEL_COUNT; wheel++) setWheelTarget(wheel, 0);
  if (lineLength) Serial.println();  // do not print over a partly typed command...
  printMoveCounts("done");
  if (lineLength) Serial.write((const uint8_t *)line, lineLength);  // ...and reprint it
  moveActive = false;
}

void cancelTimedMove() {
  moveActive = false;
}

void serialStop() {
  // Print "stopped" only if something was moving. A program that echoes the robot's output
  // back therefore cannot start an endless exchange of "stopped" lines.
  if (moveActive) printMoveCounts("stopped");
  else if (motorsBusy()) Serial.println("   stopped");
  stopRobot();
  moveActive = false;
  phoneStandDown();
}

bool someoneTyped() {
  return typedSomething;
}

// True for the \n of an Enter sent as \r\n: the \r was already the Enter. A \n that arrives
// alone, later (for example, after the line ending is switched from Carriage Return to New
// Line), is a separate Enter.
bool isRestOfEnter(char c, unsigned long now) {
  bool restOfEnter = c == '\n' && previousChar == '\r' && now - previousCharMs < CR_LF_GAP_MS;
  previousChar = c;
  previousCharMs = now;
  return restOfEnter;
}

// ---- The test command ----

// Keep the motors ramping for `ms`. Returns false as soon as a key is pressed.
// (The newline half of the Enter that started the test does not count as a key.)
bool waitWhileTesting(unsigned long ms) {
  unsigned long start = millis();
  unsigned long lastTick = millis();
  while (millis() - start < ms) {
    if (Serial.available()) {
      char c = Serial.read();
      if (isRestOfEnter(c, millis())) continue;
      while (Serial.available()) Serial.read();
      return false;
    }
    if (millis() - lastTick >= 10) {
      lastTick = millis();
      updateMotors();
      updateWheelSpeeds(lastTick);
    }
    delay(1);
  }
  return true;
}

// Run each motor on its own, forward then backward, and check its encoder.
void runTest() {
  stopRobot();
  moveActive = false;
  phoneStandDown();
  say("   test: each motor alone at +%d%% then -%d%% for %lu ms, Enter aborts\r\n",
      TEST_SPEED, TEST_SPEED, 800UL);

  for (int wheel = 0; wheel < WHEEL_COUNT; wheel++) {
    for (int sign = 1; sign >= -1; sign -= 2) {
      long start = encoderCount(wheel);
      setWheelTarget(wheel, sign * TEST_SPEED / 100.0);
      bool ok = waitWhileTesting(800);
      setWheelTarget(wheel, 0);
      ok = ok && waitWhileTesting(400);  // allow it to slow down before reading the count
      if (!ok) {
        stopRobot();
        Serial.println("   test aborted, motors stopped");
        return;
      }

      long moved = encoderCount(wheel) - start;
      say("  %s %c%d%%  %+6ld counts  ", WHEEL_PINS[wheel].name, sign > 0 ? '+' : '-', TEST_SPEED,
          moved);
      sayNumber(moved / COUNTS_PER_WHEEL_TURN, 2);
      Serial.print(" turns");
      if (labs(moved) < 20) {
        Serial.print("   <- no pulses: check encoder A/B, its power and GND, and the battery");
      } else if ((moved > 0) != (sign > 0)) {
        Serial.print("   <- counts backwards: swap A/B or set flipEncoder to true in config.h");
      }
      Serial.println();
    }
  }
  Serial.println("   test done");
}

// ---- Reading the commands ----

// Any text that is not a valid command stops the robot, and the robot prints the reason.
void reject(const char *why) {
  serialStop();
  say("   %s - type help\r\n", why);
}

bool argumentCountIs(int count, int lowest, int highest) {
  if (count >= lowest && count <= highest) return true;
  reject("wrong number of arguments");
  return false;
}

bool readTime(float value, unsigned long &ms) {
  if (value < 0) {
    reject("time must be 0 or more");
    return false;
  }
  if (value > 86400000.0) value = 86400000.0;  // limited to one day
  ms = (unsigned long)value;
  return true;
}

// The optional [speed] [time] after a move. Speed is limited to lowest..highest.
bool readSpeedAndTime(int count, const float *numbers, int lowest, int highest, int &speed,
                      unsigned long &ms) {
  if (!argumentCountIs(count, 0, 2)) return false;
  speed = defaultSpeed;
  if (count > 0) speed = (int)lround(constrain(numbers[0], (float)lowest, (float)highest));
  ms = defaultTimeMs;
  if (count > 1) return readTime(numbers[1], ms);
  return true;
}

void startMotion(const char *label, float forward, float left, float turn, int count,
                 const float *numbers) {
  int speed;
  unsigned long ms;
  if (count > 0 && numbers[0] < 0) {
    reject("speed must be 0 to 100 (to go the other way, use the opposite letter)");
    return;
  }
  if (!readSpeedAndTime(count, numbers, 0, 100, speed, ms)) return;
  phoneStandDown();
  driveRobot(forward * speed / 100.0, left * speed / 100.0, turn * speed / 100.0);
  startTimedMove(label, speed, ms);
}

void runOneMotor(int wheel, int count, const float *numbers) {
  int speed;
  unsigned long ms;
  if (count == 0) {  // a single motor needs its power, for example m1 40 or m1 -40
    char why[48];
    snprintf(why, sizeof why, "usage: %s <-100..100> [time]", WHEEL_PINS[wheel].name);
    reject(why);
    return;
  }
  if (!readSpeedAndTime(count, numbers, -100, 100, speed, ms)) return;
  phoneStandDown();
  for (int i = 0; i < WHEEL_COUNT; i++) setWheelTarget(i, 0);
  setWheelTarget(wheel, speed / 100.0);
  char label[16];
  snprintf(label, sizeof label, "%s only", WHEEL_PINS[wheel].name);
  startTimedMove(label, speed, ms);
}

void printEncoders() {
  for (int wheel = 0; wheel < WHEEL_COUNT; wheel++) {
    long count = encoderCount(wheel);
    say("  %s  %+8ld counts  ", WHEEL_PINS[wheel].name, count);
    sayNumber(count / COUNTS_PER_WHEEL_TURN, 2);
    Serial.print(" turns  ");
    sayNumber(wheelRpm(wheel), 1);
    say(" rpm   A=%d B=%d\r\n", encoderPinLevel(wheel, true), encoderPinLevel(wheel, false));
  }
}

void printHelp() {
  Serial.println();
  Serial.println("  Commands - type one, then press Enter. [speed] in %, [time] in ms");
  Serial.println("  w  forward        s  back             (also f / b)");
  Serial.println("  a  slide left     d  slide right      (also l / r)");
  Serial.println("  q  turn left      e  turn right       (anticlockwise / clockwise)");
  Serial.println("     examples:  w     w 50     q 40 2000");
  Serial.println("  go <angle> [speed] [time]   any direction:");
  Serial.println("                              0 front, 90 left, 180 back, 270 right");
  say("  %s / %s / %s <-100..100> [time]   one motor on its own\r\n", WHEEL_PINS[0].name,
      WHEEL_PINS[1].name, WHEEL_PINS[2].name);
  Serial.println("  Enter on an empty line, or x   stop now"
                 " (so does any line that is not a valid command)");
  say("  speed <0-100>   default speed      (now %d)\r\n", defaultSpeed);
  say("  time <ms>       default move time  (now %lu, 0 = until stopped)\r\n", defaultTimeMs);
  Serial.println("  enc    encoder counts, turns, rpm    zero   reset the counts");
  Serial.println("  test   spin each motor + and -, check its encoder (lift the wheels first)");
  Serial.println("  help   this list");
#if USE_PHONE_JOYSTICK
  Serial.println("  WiFi: type  wifi  for the status and the web joystick address");
  Serial.println("     to join a network:  wifi <name> <password>   (saved on the robot)");
  Serial.println("     to use the robot's own hotspot instead:  wifi off");
#endif
  say("  (this program is set up for: %s)\r\n", BOARD_LABEL);
}

void handleLine(char *text) {
  // wifi takes text (a network name and a password), so it skips the number checks.
  // A line never starts with a space: a Space at that position stops the robot instead.
  if (strncasecmp(text, "wifi", 4) == 0 && (text[4] == 0 || text[4] == ' ')) {
    wifiCommand(text + 4);
    return;
  }

  // Split the line into words: the command, then up to four numbers.
  char *word[5];
  int wordCount = 0;
  for (char *w = strtok(text, " \t"); w; w = strtok(NULL, " \t")) {
    if (wordCount == 5) {
      reject("too many arguments");
      return;
    }
    word[wordCount++] = w;
  }
  if (wordCount == 0) {  // Enter on an empty line
    serialStop();
    return;
  }

  char *command = word[0];
  for (char *c = command; *c; c++) *c = tolower(*c);
  if (strcmp(command, "x") == 0 || strcmp(command, "stop") == 0) {
    serialStop();
    return;
  }

  // Every argument must be a plain number, so that stray text (such as the robot's own
  // output echoed back) can never become a command.
  int count = wordCount - 1;
  float numbers[4];
  for (int i = 0; i < count; i++) {
    char *end;
    numbers[i] = strtod(word[i + 1], &end);
    bool notANumber = end == word[i + 1] || *end != 0 || !(fabs(numbers[i]) < 1e9);
    if (notANumber) {
      char why[48];
      snprintf(why, sizeof why, "'%.20s' is not a number", word[i + 1]);
      reject(why);
      return;
    }
  }

  for (int m = 0; m < MOTION_COUNT; m++) {
    for (int n = 0; n < 4; n++) {
      const char *name = MOTIONS[m].names[n];
      if (name && strcmp(command, name) == 0) {
        startMotion(MOTIONS[m].label, MOTIONS[m].forward, MOTIONS[m].left, MOTIONS[m].turn, count,
                    numbers);
        return;
      }
    }
  }

  for (int wheel = 0; wheel < WHEEL_COUNT; wheel++) {
    if (strcasecmp(command, WHEEL_PINS[wheel].name) == 0) {
      runOneMotor(wheel, count, numbers);
      return;
    }
  }

  if (strcmp(command, "go") == 0) {
    if (count == 0) {
      reject("usage: go <angle> [speed] [time], angle 0 front, 90 left, 180 back, 270 right");
      return;
    }
    float angle = numbers[0] * DEG_TO_RAD;
    char label[32];
    snprintf(label, sizeof label, "go %.20s deg", word[1]);
    startMotion(label, cos(angle), sin(angle), 0, count - 1, numbers + 1);
  } else if (strcmp(command, "speed") == 0) {
    if (!argumentCountIs(count, 0, 1)) return;
    if (count) defaultSpeed = (int)lround(constrain(numbers[0], 0.0f, 100.0f));
    say("   speed %d%%\r\n", defaultSpeed);
  } else if (strcmp(command, "time") == 0) {
    if (!argumentCountIs(count, 0, 1)) return;
    if (count && !readTime(numbers[0], defaultTimeMs)) return;
    if (defaultTimeMs) say("   moves last %lu ms\r\n", defaultTimeMs);
    else Serial.println("   moves last until stopped (press Enter)");
  } else if (strcmp(command, "enc") == 0) {
    if (argumentCountIs(count, 0, 0)) printEncoders();
  } else if (strcmp(command, "zero") == 0) {
    if (!argumentCountIs(count, 0, 0)) return;
    resetEncoders();
    for (int wheel = 0; wheel < WHEEL_COUNT; wheel++) countAtMoveStart[wheel] = 0;
    Serial.println("   encoder counts reset");
  } else if (strcmp(command, "test") == 0) {
    if (argumentCountIs(count, 0, 0)) runTest();
  } else if (strcmp(command, "help") == 0 || strcmp(command, "h") == 0 ||
             strcmp(command, "?") == 0) {
    if (argumentCountIs(count, 0, 0)) printHelp();
  } else {
    char why[48];
    snprintf(why, sizeof why, "unknown command '%.20s'", command);
    reject(why);
  }
}

void readSerialCommands() {
  while (Serial.available()) {
    char c = Serial.read();
    unsigned long now = millis();
    unsigned long gap = now - previousCharMs;  // since the character before this one
    typedSomething = true;
    hintGiven = false;
    if (isRestOfEnter(c, now)) continue;

    if (c == '\r' || c == '\n') {  // Enter: run the line
      if (lineLength) Serial.println();
      line[lineLength] = 0;
      lineLength = 0;
      if (lineTooLate) {  // it waited too long: do not run it, stop instead
        lineTooLate = false;
        serialStop();
        Serial.println("   that line waited too long for its Enter, so it didn't run:"
                       " send it again");
      } else {
        handleLine(line);
      }
    } else if (c == ' ' && lineLength == 0) {  // Space at the start of a line: stop
      serialStop();
    } else if (c == '\b' || c == 127) {  // Backspace
      if (lineLength) {
        lineLength--;
        Serial.print("\b \b");
        if (lineLength == 0) lineTooLate = false;  // the late line is fully erased
      }
    } else if (isprint((unsigned char)c) && lineLength < (int)sizeof(line) - 1) {
      if (lineLength == 0) lineTypedByHand = false;
      else if (gap > KEY_GAP_MS) lineTypedByHand = true;
      line[lineLength++] = c;
      Serial.print(c);  // echo the typed character
    }
  }

  // A line that arrived all at once and still has no line ending will never receive one.
  // Stop the robot (as for any invalid line), print what to change, and make sure that the
  // line cannot run later, if an Enter arrives. Printed once per batch of characters.
  if (lineLength && !hintGiven && (lineTooLate || !lineTypedByHand) &&
      millis() - previousCharMs >= LINE_WAIT_MS) {
    lineTooLate = true;
    hintGiven = true;
    Serial.println();
    serialStop();
    Serial.println("   (that line came without a line ending, so it won't run: set the Serial"
                   " Monitor's line ending, next to the baud rate, to New Line)");
  }
}
