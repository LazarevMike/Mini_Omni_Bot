// Runs the real robot program against simulated motors + encoders and checks behaviour.
#include "Arduino.h"
#include <algorithm>
uint32_t g_ms = 0;
bool g_hostReading = false;
uint32_t g_maxPwmSeen = 0;
bool g_moveSeen = false;
std::map<int, int> g_pinLevel;
std::map<int, int> g_pinInput;
std::map<int, int> g_analog;
std::map<int, void (*)()> g_isr;
MockSerial Serial;
void simTick1ms();
std::multimap<uint32_t, std::string> g_scheduled;   // keystrokes that arrive at a set time
void at(uint32_t delayMs, const char *s) { g_scheduled.emplace(g_ms + delayMs, s); }
void simAdvance(uint32_t ms) {
  while (ms--) {
    g_ms++;
    while (!g_scheduled.empty() && g_scheduled.begin()->first <= g_ms) {
      for (char c : g_scheduled.begin()->second) Serial.in.push_back(c);
      g_scheduled.erase(g_scheduled.begin());
    }
    simTick1ms();
  }
}

#include "motors.cpp"
#include "encoders.cpp"
#include "drive.cpp"
#include "serial_commands.cpp"
#include "phone_control.cpp"
#include "OmniBots_Mini.ino"

#if USE_PHONE_JOYSTICK
// wifi_server.cpp stand-ins: the host test drives the phone logic directly
int g_wifiCalls = 0;
std::string g_wifiArgs;
void setupWifi() {}
void updateWifi() {}
void wifiCommand(const char *args) {
  g_wifiCalls++;
  g_wifiArgs = args;
}
#else
#include "wifi_server.cpp"   // phone joystick off: the real stand-ins, nothing else
#endif

// ---- simulation -------------------------------------------------------------
double simPos[8];
long simStep[8];
int simEncSign[8] = {1, 1, 1, 1, 1, 1, 1, 1};   // -1 = A/B wired the other way round
bool simEncConnected[8] = {true, true, true, true, true, true, true, true};
const double COUNTS_PER_MS_FULL = 350.0 * 1496 / 60000.0;
const uint8_t AB_SEQ[4] = {0b00, 0b10, 0b11, 0b01};

void setPin(int pin, int v) { g_pinInput[pin] = v; }

uint32_t pwmFor(int i) { return g_analog[WHEEL_PINS[i].pin1]; }  // PH/EN: EN carries the speed

double applied(int i) {   // signed output the motor receives, -1..1
  double mag = pwmFor(i) / (double)PWM_MAX;
  return g_pinLevel[WHEEL_PINS[i].pin2] ? mag : -mag;
}

void simTick1ms() {
  for (int i = 0; i < WHEEL_COUNT; i++) {
    if (pwmFor(i) > g_maxPwmSeen) g_maxPwmSeen = pwmFor(i);
    if (moveActive) g_moveSeen = true;
    simPos[i] += applied(i) * COUNTS_PER_MS_FULL * simEncSign[i];
    long target = (long)floor(simPos[i]);
    while (simStep[i] != target && simEncConnected[i]) {
      long next = simStep[i] + (target > simStep[i] ? 1 : -1);
      uint8_t before = AB_SEQ[((simStep[i] % 4) + 4) % 4], after = AB_SEQ[((next % 4) + 4) % 4];
      simStep[i] = next;
      const WheelPins &w = WHEEL_PINS[i];
      setPin(w.encA, after >> 1);
      setPin(w.encB, after & 1);
      int changed = (before ^ after) & 2 ? w.encA : w.encB;
      g_isr[changed]();
    }
    if (!simEncConnected[i]) simStep[i] = target;
  }
}

// ---- helpers -----------------------------------------------------------------
int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s  (%s)\n", msg, #cond); failures++; } else printf("ok:   %s\n", msg); } while (0)

void type(const char *s) { for (const char *p = s; *p; p++) Serial.in.push_back(*p); }
void run(uint32_t ms) { for (uint32_t t = 0; t < ms; t++) { loop(); simAdvance(1); } }
std::string takeOut() { std::string o = Serial.out; Serial.out.clear(); return o; }
bool has(const std::string &s, const char *needle) { return s.find(needle) != std::string::npos; }

// body velocity from the wheel pushes (positive push = anticlockwise tangent)
void bodyFromWheels(double out[3]) {
  double J[3][3], inv[3][3], p[3];
  for (int i = 0; i < 3; i++) {
    double a = WHEEL_SETUP[i].angle * M_PI / 180;
    J[i][0] = -sin(a); J[i][1] = cos(a); J[i][2] = 1;
    p[i] = applied(i) * (WHEEL_SETUP[i].flipMotor ? -1 : 1);
  }
  double det = J[0][0] * (J[1][1] * J[2][2] - J[1][2] * J[2][1]) - J[0][1] * (J[1][0] * J[2][2] - J[1][2] * J[2][0]) + J[0][2] * (J[1][0] * J[2][1] - J[1][1] * J[2][0]);
  for (int r = 0; r < 3; r++) for (int c = 0; c < 3; c++) {
    int r1 = (c + 1) % 3, r2 = (c + 2) % 3, c1 = (r + 1) % 3, c2 = (r + 2) % 3;
    inv[r][c] = (J[r1][c1] * J[r2][c2] - J[r1][c2] * J[r2][c1]) / det;
  }
  for (int r = 0; r < 3; r++) out[r] = inv[r][0] * p[0] + inv[r][1] * p[1] + inv[r][2] * p[2];
}

void showBody(const char *label) {
  double b[3]; bodyFromWheels(b);
  printf("      %-14s body vx %+.2f vy %+.2f wz %+.2f | pwm M1 %4u M3 %4u M4 %4u | PH %d %d %d\n", label, b[0], b[1], b[2],
         pwmFor(0), pwmFor(1), pwmFor(2), g_pinLevel[WHEEL_PINS[0].pin2], g_pinLevel[WHEEL_PINS[1].pin2], g_pinLevel[WHEEL_PINS[2].pin2]);
}

int main() {
  // The simulated encoders are wired like the real robot's: where config.h says an encoder
  // counts backwards (flipEncoder), the simulated one does too.
  for (int i = 0; i < WHEEL_COUNT; i++) simEncSign[i] = WHEEL_SETUP[i].flipEncoder ? -1 : 1;
  setup();
  g_hostReading = true;
  run(1000);
  std::string o = takeOut();
  CHECK(o.empty(), "nothing is printed unasked, so a monitor has nothing to echo back");
  CHECK(pwmFor(0) == 0 && pwmFor(1) == 0 && pwmFor(2) == 0, "motors stopped at boot");
  auto phLow = []() { return !g_pinLevel[WHEEL_PINS[0].pin2] && !g_pinLevel[WHEEL_PINS[1].pin2] &&
                             !g_pinLevel[WHEEL_PINS[2].pin2]; };
  CHECK(phLow(), "stopped, the direction (PH) pins are low too");
  type("help\n");
  run(5);
  o = takeOut();
  CHECK(has(o, "Commands - type one") && has(o, "turn left"), "help lists the commands");
  std::string transcript = o.substr(5);   // what the robot printed (minus the echoed "help"), for the echo test

  // forward, CRLF line ending must not turn into an extra empty-line stop
  type("w\r\n"); run(300);
  o = takeOut(); showBody("w @300ms");
  double b[3]; bodyFromWheels(b);
  CHECK(has(o, "-> forward 30% for 1000 ms") && !has(o, "stopped"), "w starts a 1 s move, CRLF ok");
  CHECK(b[0] > 0.2 && fabs(b[1]) < 0.02 && fabs(b[2]) < 0.02, "w drives straight forward");
  CHECK(pwmFor(1) == 0, "w leaves the back wheel idle");
  CHECK(fabs(pwmFor(0) - 0.3 * PWM_MAX) < 2 && fabs(pwmFor(2) - 0.3 * PWM_MAX) < 2, "w front wheels at 30%");
  run(900);
  o = takeOut(); printf("%s", o.c_str());
  CHECK(has(o, "done"), "move ends by itself after 1 s");
  run(200);
  CHECK(pwmFor(0) == 0 && pwmFor(2) == 0, "ramped to zero after the move");

  // the other directions
  struct { const char *cmd; double vx, vy, wz; } dirs[] = {
    {"s", -1, 0, 0}, {"a", 0, 1, 0}, {"d", 0, -1, 0}, {"q", 0, 0, 1}, {"e", 0, 0, -1},
    {"go 45", 1, 1, 0}, {"go 270 60", 0, -1, 0}, {"forward 80 300", 1, 0, 0}};
  for (auto &d : dirs) {
    type(d.cmd); type("\n"); run(150); bodyFromWheels(b); showBody(d.cmd);
    double n = sqrt(b[0] * b[0] + b[1] * b[1] + b[2] * b[2]), m = sqrt(d.vx * d.vx + d.vy * d.vy + d.wz * d.wz);
    double cosang = (b[0] * d.vx + b[1] * d.vy + b[2] * d.wz) / (n * m);
    char msg[64]; snprintf(msg, sizeof msg, "direction of '%s'", d.cmd);
    CHECK(cosang > 0.999, msg);
    type("x\n"); run(5); takeOut();
  }
  CHECK(pwmFor(0) == 0 && pwmFor(1) == 0 && pwmFor(2) == 0, "x stops at once");
  CHECK(phLow(), "...and the direction (PH) pins go low");

  // speed clamp, continuous mode and the stop keys
  type("speed 150\n"); run(5); o = takeOut();
  CHECK(has(o, "speed 100%"), "speed clamps to 100");
  type("speed 40\ntime 0\n"); run(5); o = takeOut();
  CHECK(has(o, "until stopped"), "time 0 = until stopped");
  type("q\n"); run(3000);
  CHECK(pwmFor(0) > 0.39 * PWM_MAX, "continuous move still running after 3 s");
  type(" "); run(2); o = takeOut();
  CHECK(has(o, "stopped") && pwmFor(0) == 0, "space stops immediately");
  type("q\n"); run(200); type("\n"); run(2); o = takeOut();
  CHECK(has(o, "stopped") && pwmFor(0) == 0, "Enter on an empty line stops");
  type("time 1000\nspeed 30\n"); run(5); takeOut();

  // ---- line endings, sent the way each Serial Monitor sends them ----
  // Arduino IDE: the whole line on Enter, plus New Line (\n, its default), Carriage Return
  // (\r) or Both NL & CR (\r\n). PlatformIO with send_on_enter: the whole line plus \n.
  struct { const char *name, *end; } endings[] = {
    {"New Line", "\n"}, {"Carriage Return", "\r"}, {"Both NL & CR", "\r\n"}};
  for (auto &e : endings) {
    char msg[96];
    std::string cmd = std::string("help") + e.end;
    type(cmd.c_str()); run(5); o = takeOut();
    snprintf(msg, sizeof msg, "%s: help runs once", e.name);
    CHECK(has(o, "Commands - type one") && o.find("Commands - type one") == o.rfind("Commands - type one") &&
          !has(o, "unknown"), msg);
    cmd = std::string("w 30 0") + e.end;
    type(cmd.c_str()); run(300); o = takeOut();
    snprintf(msg, sizeof msg, "%s: a move starts, and its line ending adds no stop", e.name);
    CHECK(has(o, "-> forward 30% until stopped") && !has(o, "   stopped") && pwmFor(0) > 0, msg);
    type(e.end); run(5); o = takeOut();
    snprintf(msg, sizeof msg, "%s: Enter on its own stops", e.name);
    CHECK(has(o, "   stopped") && pwmFor(0) == 0, msg);
  }

  // No Line Ending: the line arrives all at once and never gets its Enter. It never runs:
  // after 1.5 s the robot stops and says why, once.
  g_moveSeen = false; g_maxPwmSeen = 0;
  type("w"); run(1400); o = takeOut();
  CHECK(!g_moveSeen && g_maxPwmSeen == 0 && !has(o, "line ending"), "No Line Ending: nothing runs while it waits");
  run(200); o = takeOut();
  CHECK(has(o, "   (that line came without a line ending, so it won't run") && !g_moveSeen && g_maxPwmSeen == 0,
        "No Line Ending: after 1.5 s the robot says the line won't run, and how to fix it");
  run(3000); o = takeOut();
  CHECK(o.empty(), "the hint is printed once, not over and over");
  // the user switches to New Line: the late line is refused once, then lines run again
  type("w\n"); run(100); o = takeOut();
  CHECK(has(o, "waited too long for its Enter, so it didn't run") && !g_moveSeen && g_maxPwmSeen == 0,
        "the late line never runs, even when an Enter comes for it later");
  type("w\n"); run(100); o = takeOut();
  CHECK(has(o, "-> forward 30%") && !has(o, "unknown") && !has(o, "too long"), "the next line runs as normal");
  type("x\n"); run(5); takeOut();
  type("w 30 0\n"); run(300); takeOut();
  type("x"); run(1600); o = takeOut();
  CHECK(has(o, "   stopped") && has(o, "without a line ending") && pwmFor(0) == 0,
        "No Line Ending: a line left waiting stops a running move");
  type("help"); run(1600); o = takeOut();
  CHECK(has(o, "without a line ending") && !has(o, "Commands"), "...and the hint comes again for the next line sent");
  type("\n"); run(5); o = takeOut();
  CHECK(has(o, "too long") && !has(o, "Commands") && !has(o, "unknown"), "Enter on the late lines refuses them, as one");

  // A late line erased with Backspace is gone: the next typed text is a new line.
  type("w"); run(1600); takeOut();
  type("\x7f" "enc\n"); run(5); o = takeOut();
  CHECK(has(o, "counts") && !has(o, "too long"), "a late line rubbed out with Backspace doesn't block the next one");

  // A serial program that sends each key as it is pressed: nothing runs before Enter, and
  // a line typed by hand has no time limit.
  g_moveSeen = false; g_maxPwmSeen = 0;
  at(0, "w"); at(400, " "); at(2400, "8"); at(2800, "0"); at(3200, "\n");
  run(3199); o = takeOut();
  CHECK(!g_moveSeen && g_maxPwmSeen == 0 && !has(o, "line ending"),
        "typed slowly, key by key: nothing runs before Enter, and no hint");
  run(200); o = takeOut();
  CHECK(has(o, "-> forward 80%") && !has(o, "too long"), "...and it runs when Enter comes");
  type("x\n"); run(5); takeOut();
  // ...but a single key followed by a long pause cannot be distinguished from No Line
  // Ending: that line is refused, and so is the rest typed after the pause. Nothing runs.
  struct { const char *first, *rest; } splits[] = {
    {"w", " 80\n"}, {"c", "cw 50\n"}, {"c", "w 60\n"}, {"t", "ime 2000\n"}};
  for (auto &sp : splits) {
    g_moveSeen = false; g_maxPwmSeen = 0;
    at(0, sp.first); at(2000, sp.rest);
    run(2500); o = takeOut();
    char msg[96];
    snprintf(msg, sizeof msg, "'%s', a long pause, then '%.*s' and Enter: nothing runs", sp.first,
             (int)strcspn(sp.rest, "\n"), sp.rest);
    CHECK(!g_moveSeen && g_maxPwmSeen == 0 && has(o, "without a line ending") && has(o, "too long") &&
          !has(o, "-> ") && defaultTimeMs == 1000, msg);
  }
  // a word typed by hand with a long pause in the middle runs as typed
  g_moveSeen = false;
  at(0, "t"); at(150, "i"); at(300, "m"); at(2400, "e"); at(2550, " "); at(2700, "4"); at(2850, "0"); at(3000, "0"); at(3150, "\n");
  run(3300); o = takeOut();
  CHECK(has(o, "moves last 400 ms") && !has(o, "line ending") && !g_moveSeen, "'tim', a long pause, then 'e 400': runs as typed");
  type("time 1000\n"); run(5); takeOut();

  // Carriage Return, then the line ending switched to New Line: the later \n is an Enter
  // of its own, so a bare Enter still stops.
  type("time 0\r"); type("w\r"); run(500); takeOut();
  CHECK(pwmFor(0) > 0, "Carriage Return: a move runs");
  type("\n"); run(5); o = takeOut();
  CHECK(has(o, "stopped") && pwmFor(0) == 0, "a \\n on its own, long after a \\r, is an Enter: it stops");
  type("time 1000\n"); run(5); takeOut();

  // encoder counting: one wheel turn at the commanded direction
  type("zero\n"); run(5); long s3 = simStep[1]; type("m3 50 2000\n"); run(2400); o = takeOut(); printf("%s", o.c_str());
  type("enc\n"); run(5); o = takeOut(); printf("%s", o.c_str());
  long c3 = encoderCount(1);
  CHECK(c3 > 0 && labs(c3 - (simStep[1] - s3) * (WHEEL_SETUP[1].flipEncoder ? -1 : 1)) <= 1, "x4 decoder matches simulated steps");
  CHECK(has(o, "M3") && has(o, "turns"), "enc prints counts and turns");

  // backspace editing
  type("enx\x7f" "c\n"); run(5); o = takeOut();
  CHECK(has(o, "counts") && !has(o, "unknown"), "backspace edits the line");
  type("jump\n"); run(5); o = takeOut();
  CHECK(has(o, "unknown command 'jump'"), "unknown command reported");

  // test command: healthy, swapped A/B on M4, unplugged M3 encoder
  simEncSign[2] = -simEncSign[2]; simEncConnected[1] = false;
  type("test\n"); run(10); o = takeOut(); printf("%s", o.c_str());
  CHECK(has(o, "test done"), "test runs to the end");
  CHECK(has(o, "M4 +25%") && has(o, "counts backwards"), "test flags swapped encoder");
  CHECK(has(o, "no pulses"), "test flags missing encoder");
  size_t m1 = o.find("M1 +25%"); size_t m1e = o.find('\n', m1);
  CHECK(m1 != std::string::npos && o.substr(m1, m1e - m1).find("<-") == std::string::npos, "healthy M1 has no warning");
  simEncSign[2] = -simEncSign[2]; simEncConnected[1] = true;

  // abort the test with a key
  type("test\nx"); run(10); o = takeOut();
  CHECK(has(o, "test aborted") && pwmFor(0) == 0, "any key aborts the test");
  run(5); takeOut();

  // monitors can send Enter as CR+LF; the LF must not abort the test it started
  // (the test blocks while it runs, so keys that come later are scheduled)
  type("test\r\n"); run(1); o = takeOut();
  CHECK(has(o, "test done") && !has(o, "test aborted"), "test started with CR+LF runs to the end");
  type("test\r"); at(20, "\n"); run(1); o = takeOut();
  CHECK(has(o, "test done") && !has(o, "test aborted"), "LF arriving a moment after the CR is ignored too");
  type("test\r\n"); at(300, "\r\n"); run(1); o = takeOut();
  CHECK(has(o, "test aborted") && !has(o, "test done") && pwmFor(0) == 0, "a later Enter aborts the test");
  type("test\r\n"); at(2000, " "); run(1); o = takeOut();
  CHECK(has(o, "test aborted") && pwmFor(1) == 0, "Space aborts the test mid-way");
  run(10); takeOut();

  // a new command while a move runs takes over and restarts the timer
  type("w\n"); run(500); type("a\n"); run(700); o = takeOut();
  CHECK(!has(o, "done"), "second command restarts the move timer");
  run(400); o = takeOut();
  CHECK(has(o, "done"), "second move ends on its own timer");

  // strict arguments: words, units and junk never become numbers
  struct { const char *line, *expect; } bad[] = {
    {"w forward", "'forward' is not a number"}, {"m1 +25%", "'+25%' is not a number"},
    {"q 40 2000 9", "wrong number of arguments"}, {"test done", "'done' is not a number"},
    {"test 1", "wrong number of arguments"}, {"speed abc", "'abc' is not a number"},
    {"time -5", "time must be 0 or more"}, {"go", "usage: go"}, {"w nan", "'nan' is not a number"},
    {"a b c d e f", "too many arguments"}};
  for (auto &t : bad) {
    type(t.line); type("\n"); run(20);
    o = takeOut();
    char msg[80]; snprintf(msg, sizeof msg, "'%s' is rejected", t.line);
    CHECK(has(o, t.expect) && !has(o, "-> ") && pwmFor(0) == 0, msg);
  }
  CHECK(defaultSpeed == 30 && defaultTimeMs == 1000, "rejected lines leave the settings alone");
  type("w 50 0\n"); run(300); type("stpo\n"); run(2); o = takeOut();
  CHECK(has(o, "stopped") && has(o, "unknown command 'stpo'") && pwmFor(0) == 0, "a typo stops a running move");
  type("x 5\n"); run(2); type("w\n"); run(100); type("x now\n"); run(2); o = takeOut();
  CHECK(pwmFor(0) == 0 && has(o, "stopped"), "x stops even with junk after it");
  type("m1\n"); run(200); o = takeOut();
  CHECK(has(o, "usage: M1 <-100..100> [time]") && pwmFor(0) == 0, "a motor command without its power is rejected");
  type("w -50\n"); run(200); o = takeOut();
  CHECK(has(o, "speed must be 0 to 100") && pwmFor(0) == 0 && pwmFor(2) == 0, "a negative speed on a move is rejected");
  type("\n\n \n"); run(5); o = takeOut();
  CHECK(o.empty(), "stopping while already stopped prints nothing");

  // a monitor echoing the robot's own output back must never move the robot
  for (const char *cmd : {"help", "enc", "speed 40", "time 1500", "w 30 400", "m3 -20 300",
                          "go 45", "zero", "bogus", "go", "test"}) {
    type(cmd); type("\n"); run(cmd[0] == 't' && cmd[1] == 'e' ? 10 : 1700);
    o = takeOut();
    if (o.compare(0, strlen(cmd) + 1, std::string(cmd) + "\n") == 0) o = o.substr(strlen(cmd) + 1);
    transcript += o;
  }
  simEncSign[2] = -simEncSign[2]; simEncConnected[1] = false;   // so the replay holds the warning lines too
  type("test\n"); run(10); o = takeOut(); transcript += o.substr(5);
  simEncSign[2] = -simEncSign[2]; simEncConnected[1] = true;
  type("w 30 0\n"); run(200); takeOut();   // a move, so the hint below comes with "stopped" lines
  type("bogus"); run(1700); o = takeOut(); transcript += o.substr(5);   // No Line Ending's hint
  type("\n"); run(5); o = takeOut(); transcript += o;                   // ...and the late line refused
  type("speed 30\ntime 1000\n"); run(5); takeOut();
  CHECK(has(transcript, "test done") && has(transcript, "M1 +25%") && has(transcript, "counts backwards") &&
        has(transcript, "without a line ending") && has(transcript, "too long"),
        "echo transcript covers moves, tests, warnings, the line-ending hint and a refused late line");
  g_maxPwmSeen = 0;
  g_moveSeen = false;
  for (char c : transcript) Serial.in.push_back(c);
  run(3000);
  o = takeOut();
  CHECK(g_maxPwmSeen == 0, "echoed output never drives a motor");
  CHECK(!g_moveSeen, "echoed output never starts a move or the test");
  CHECK(defaultSpeed == 30 && defaultTimeMs == 1000, "echoed output never changes the settings");

#if USE_PHONE_JOYSTICK
  // ---- web joystick ----
  run(300); takeOut();
  CHECK(g_wifiCalls == 0, "echoed output never reaches the wifi command");
  auto msg = [](uint32_t client, const char *json) { return phoneMessage(client, json, g_ms, g_ms); };
  auto drive = [](float x, float y, float r, int spin, int limit) {
    static char json[128];
    snprintf(json, sizeof json, "{\"type\":\"drive\",\"x\":%g,\"y\":%g,\"r\":%g,\"spin\":%d,\"limit\":%d}", x, y, r, spin, limit);
    return (const char *)json;
  };
  // a page streaming its drive state like the real one: every 50 ms
  auto stream = [&](const char *json, uint32_t ms) {
    std::string copy = json;
    for (uint32_t t = 0; t < ms; t += 50) { msg(7, copy.c_str()); run(50); }
  };
  char tel[220];
  CHECK(phoneConnected(7, g_ms) && !phoneConnected(8, g_ms), "the first page gets control, a second one is turned away");
  phoneStatus(tel, sizeof tel, false, -56);
  CHECK(has(tel, "\"limit\":50"), "a new page starts at a 50% limit");
  stream(drive(0, 1, 0, 0, 50), 300);
  CHECK(pwmFor(0) == 0 && pwmFor(2) == 0, "drive states are ignored while Joystick mode is off");
  msg(7, "{\"type\":\"enable_joy\"}");
  stream(drive(0, 1, 0, 0, 50), 600);
  bodyFromWheels(b); showBody("web fwd 50%");
  CHECK(b[0] > 0.2 && fabs(b[1]) < 0.02 && fabs(b[2]) < 0.02, "stick up drives forward");
  CHECK(fabs(pwmFor(0) - 0.5 * PWM_MAX) < 3 && fabs(pwmFor(2) - 0.5 * PWM_MAX) < 3,
        "full stick at a 50% limit puts the busiest wheels at 50%");
  phoneStatus(tel, sizeof tel, false, -56);
  CHECK(has(tel, "\"joy_enabled\":true") && has(tel, "\"speed\":50") && has(tel, "\"angle\":0,") &&
        has(tel, "\"spin\":0") && has(tel, "\"turn\":0") && has(tel, "\"net\":\"sta\"") && has(tel, "\"rssi\":-56"),
        "telemetry reports what is being driven");
  stream(drive(1, 0, 0, 0, 50), 600);
  bodyFromWheels(b); showBody("web right");
  phoneStatus(tel, sizeof tel, true, 0);
  CHECK(b[1] < -0.2 && fabs(b[0]) < 0.02 && has(tel, "\"angle\":270,") && has(tel, "\"net\":\"ap\"") &&
        has(tel, "\"rssi\":null"), "stick right strafes right (angle 270, counted counter-clockwise; hotspot has no signal reading)");
  stream(drive(-0.7071f, -0.7071f, 0, 0, 50), 600);
  bodyFromWheels(b);
  CHECK(b[0] < -0.1 && b[1] > 0.1 && fabs(b[0] - (-b[1])) < 0.02, "stick back-left drives back-left");
  stream(drive(0.05f, 0.03f, 0, 0, 50), 600);
  CHECK(pwmFor(0) == 0 && pwmFor(1) == 0 && pwmFor(2) == 0, "small stick travel is inside the deadzone");
  stream(drive(0, 1, 0, 0, 30), 600);
  CHECK(fabs(pwmFor(0) - 0.3 * PWM_MAX) < 3, "the limit in the drive state applies at once");
  stream(drive(0, 1, 0, 0, 50), 400);
  run(600);
  CHECK(pwmFor(0) == 0 && pwmFor(2) == 0, "drive states stop for 0.5 s: the robot stops");

  stream(drive(0, 0, 0, 1, 50), 2000);
  bodyFromWheels(b); showBody("web spin left");
  phoneStatus(tel, sizeof tel, false, -56);
  CHECK(b[2] > 0.1 && fabs(b[0]) < 0.02 && has(tel, "\"spin\":1") && has(tel, "\"turn\":25"),
        "spin Left keeps turning counter-clockwise");
  stream(drive(0, 0, 0, -1, 50), 600);
  bodyFromWheels(b);
  CHECK(b[2] < -0.1, "spin Right turns clockwise");
  for (int t = 0; t < 30; t++) { msg(7, "{\"type\":\"ping\"}"); run(50); }
  phoneStatus(tel, sizeof tel, false, -56);
  CHECK(pwmFor(0) == 0 && pwmFor(1) == 0 && has(tel, "\"spin\":0"),
        "pings alone do not keep a spin going: it stops after 0.5 s");
  // the release message is lost: later states carry the zero
  stream(drive(0, 1, 0, 0, 100), 600);
  msg(7, drive(0, 0, 0, 0, 100));
  CHECK(phone.y == 0, "a lost release is undone by the next drive state...");
  run(11);
  CHECK(wheelTarget[0] == 0 && wheelTarget[2] == 0, "...the wheel targets drop on the next tick...");
  stream(drive(0, 0, 0, 0, 100), 300);
  CHECK(pwmFor(0) == 0 && pwmFor(2) == 0, "...and the wheels ramp down to zero within 0.3 s");
  stream(drive(0, 0, 0, 1, 50), 500);
  msg(7, "{\"type\":\"estop\"}");
  run(1);
  CHECK(pwmFor(0) == 0 && pwmFor(1) == 0 && pwmFor(2) == 0 && phone.spin == 0 && phone.joystickOn,
        "estop brakes at once, drops the spin, keeps Joystick mode on");
  stream(drive(0, 0, 1, 0, 50), 600);
  bodyFromWheels(b);
  CHECK(b[2] > 0.1, "held turn (Q) turns counter-clockwise");
  stream(drive(0, 1, 1, 1, 100), 600);
  uint32_t top = std::max({pwmFor(0), pwmFor(1), pwmFor(2)});
  bodyFromWheels(b);
  CHECK(top <= PWM_MAX && top >= PWM_MAX - 2 && b[0] > 0.1 && b[2] > 0.1,
        "drive plus turn is scaled to fit 100% and keeps both motions");
  msg(7, drive(0, 0, 0, 0, 500));
  CHECK(phone.limit == 100, "limit clamps to 100");
  msg(7, drive(0, 0, 0, 0, 1));
  CHECK(phone.limit == 10, "limit clamps to 10");
  // a drive state that sat in the queue: ignored, and the deadman still runs
  msg(7, "{\"type\":\"estop\"}");
  run(100);
  phoneMessage(7, drive(0, 1, 0, 0, 100), g_ms - 400, g_ms);
  run(300);
  CHECK(pwmFor(0) == 0 && pwmFor(2) == 0, "a drive state that waited 0.4 s in the queue is ignored");
  stream(drive(0, 1, 0, 0, 100), 400);
  phoneMessagesLost();
  run(300);
  CHECK(pwmFor(0) == 0 && pwmFor(2) == 0 && phone.x == 0 && phone.y == 0, "lost page messages stop the robot");
  stream(drive(0, 1, 0, 0, 100), 400);
  CHECK(pwmFor(0) > 0, "the next drive state picks up again");
  msg(7, "{\"type\":\"estop\"}");
  for (const char *junk : {"{", "", "{\"type\":\"drive\",\"x\":\"a\",\"y\":1,\"r\":0,\"spin\":0,\"limit\":50}",
                           "{\"type\":\"drive\",\"x\":nan,\"y\":1,\"r\":0,\"spin\":0,\"limit\":50}",
                           "{\"type\":\"drive\",\"x\":1e30,\"y\":1,\"r\":0,\"spin\":0,\"limit\":50}",
                           "{\"type\":\"drive\",\"x\":0,\"y\":1,\"r\":0,\"spin\":0.5,\"limit\":50}",
                           "{\"type\":\"drive\",\"x\":0,\"y\":1,\"r\":0,\"spin\":7,\"limit\":50}",
                           "{\"type\":\"drive\",\"x\":0,\"y\":1}", "{\"type\":\"nonsense\"}", "{\"type\":1}"})
    msg(7, junk);
  run(400);
  CHECK(pwmFor(0) == 0 && pwmFor(1) == 0 && pwmFor(2) == 0 && phone.spin == 0, "malformed messages are ignored");
  CHECK(msg(7, "{\"type\":\"ping\"}") && !strcmp(msg(7, "{\"type\":\"ping\"}"), "{\"type\":\"pong\"}") &&
        !msg(8, "{\"type\":\"ping\"}"), "ping gets a pong, only for the page in control");
  CHECK(!msg(8, "{\"type\":\"enable_joy\"}") && phone.joystickOn, "other pages cannot change anything");

  // the Serial Monitor always wins
  stream(drive(0, 1, 0, 0, 50), 400);
  takeOut();
  type("\r\n"); run(2); o = takeOut();
  CHECK(!phone.joystickOn && pwmFor(0) == 0 && has(o, "web Joystick mode turned off"), "Enter in the Serial Monitor stops the page's driving");
  stream(drive(0, 1, 0, 0, 50), 300);
  CHECK(pwmFor(0) == 0, "the page's stick does nothing until Joystick mode is turned on again");
  msg(7, "{\"type\":\"enable_joy\"}");
  stream(drive(0, 1, 0, 0, 50), 200);
  type("q 40 300\n"); run(100); o = takeOut();
  CHECK(!phone.joystickOn && has(o, "-> turn left 40%"), "a move typed in the Serial Monitor takes over from the page");
  run(500); takeOut();
  type("w 30 0\n"); run(200);
  msg(7, "{\"type\":\"enable_joy\"}"); run(1);
  CHECK(!moveActive && pwmFor(0) == 0 && phone.joystickOn, "turning Joystick mode on cancels a typed move");

  // disconnects
  stream(drive(0, 1, 0, 0, 50), 400);
  phoneDisconnected(8);
  CHECK(phone.joystickOn && phone.client == 7, "a page that is not in control leaving changes nothing");
  phoneDisconnected(7); run(1);
  CHECK(!phone.joystickOn && phone.client == 0 && pwmFor(0) == 0, "the page in control leaving stops the robot and frees control");
  CHECK(phoneConnected(9, g_ms), "another page can take control after that");
  run(5000);
  CHECK(phoneGoneQuiet(g_ms) == 0, "a page quiet for 5 s keeps control");
  run(1200);
  CHECK(phoneGoneQuiet(g_ms) == 9 && phone.client == 0, "a page quiet for 6 s loses control");
  takeOut();

  // the typed wifi command
  type("wifi MyNet Example Pass1\n"); run(2); o = takeOut();
  CHECK(g_wifiCalls == 1 && g_wifiArgs == " MyNet Example Pass1" && !has(o, "unknown"),
        "wifi passes the rest of the line through, case and spaces kept");
  type("WIFI\n"); run(2);
  CHECK(g_wifiCalls == 2 && g_wifiArgs.empty(), "wifi on its own asks for the status");
  type("wifi:\n"); run(2); o = takeOut();
  CHECK(g_wifiCalls == 2 && has(o, "unknown command 'wifi:'"), "wifi: is not the wifi command");
  type("help\n"); run(5); o = takeOut();
  CHECK(has(o, "wifi <name> <password>"), "help lists the wifi commands while the phone joystick is on");

#else
  // ---- phone joystick off: no web code at all, the Serial Monitor does everything ----
  run(300); takeOut();
  type("help\n"); run(5); o = takeOut();
  CHECK(has(o, "Commands - type one") && !has(o, "wifi") && !has(o, "WiFi"),
        "help leaves the wifi lines out when the phone joystick is off");
  type("w 30 0\n"); run(300); takeOut();
  type("wifi\n"); run(5); o = takeOut();
#if defined(ESP32)
  CHECK(has(o, "   phone joystick is off: set REMOTE_CONTROL to 1 in config.h"),
        "wifi says the phone joystick is switched off (ESP32 board)");
#else
  CHECK(has(o, "   the phone joystick needs the program to run on an ESP32 (on a UNO R4 WiFi"),
        "wifi says the phone joystick needs an ESP32, and why the UNO R4 WiFi's doesn't count");
#endif
  CHECK(!has(o, "unknown") && !has(o, "   stopped") && pwmFor(0) > 0,
        "wifi is still a command, and like a plain wifi it doesn't stop the robot");
  for (const char *line : {"wifi off", "wifi MyNet Secret123", "wifi foo"}) {
    type("w 30 0\n"); run(300); takeOut();
    type(line); type("\n"); run(5); o = takeOut();
    char msg[80]; snprintf(msg, sizeof msg, "'%s' answers, and stops the robot like any change would", line);
    CHECK(!has(o, "unknown") && has(o, "   stopped") && has(o, "phone joystick") && pwmFor(0) == 0, msg);
  }
#endif

  printf("\n%s: %d failure(s)\n", failures ? "FAILED" : "ALL PASSED", failures);
  return failures != 0;
}
