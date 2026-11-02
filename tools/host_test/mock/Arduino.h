// A minimal stand-in for the Arduino core, so that the robot's program can run on a computer
// against simulated motors and encoders (see harness.cpp).
#pragma once
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <map>
#include <string>
#include <strings.h>

#define IRAM_ATTR
#define DRAM_ATTR
#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define CHANGE 3
#define DEG_TO_RAD 0.017453292519943295
#define RAD_TO_DEG 57.29577951308232
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#define digitalPinToInterrupt(p) (p)
#ifndef ESP_ARDUINO_VERSION_MAJOR
#define ESP_ARDUINO_VERSION_MAJOR 2
#endif

extern uint32_t g_ms;
void simAdvance(uint32_t ms);
inline uint32_t millis() { return g_ms; }
inline void delay(uint32_t ms) { simAdvance(ms); }
inline void noInterrupts() {}
inline void interrupts() {}

extern std::map<int, int> g_pinLevel;    // outputs written by the program
extern std::map<int, int> g_pinInput;    // inputs, driven by the simulation
extern std::map<int, int> g_analog;      // analogWrite() values per pin
extern std::map<int, void (*)()> g_isr;  // interrupt functions per pin
inline void pinMode(int, int) {}
inline void digitalWrite(int pin, int v) { g_pinLevel[pin] = v; }
inline int digitalRead(int pin) { return g_pinInput.count(pin) ? g_pinInput[pin] : 1; }
inline void analogWrite(int pin, int v) { g_analog[pin] = v; }
inline void analogWriteFrequency(uint32_t) {}       // ESP32 core 2.x, STM32
inline void analogWriteFrequency(int, uint32_t) {}  // ESP32 core 3.x
inline void attachInterrupt(int pin, void (*fn)(), int) { g_isr[pin] = fn; }

struct MockSerial {
  std::deque<char> in;
  std::string out;
  void begin(int) {}
  explicit operator bool() const { return true; }
  int available() { return (int)in.size(); }
  int read() {
    if (in.empty()) return -1;
    char c = in.front();
    in.pop_front();
    return (unsigned char)c;
  }
  void print(const char *s) { out += s; }
  void print(char c) { out += c; }
  void print(float v, int decimals) {
    char buf[32];
    snprintf(buf, sizeof buf, "%.*f", decimals, v);
    out += buf;
  }
  void println() { out += "\n"; }
  void println(const char *s) {
    out += s;
    out += "\n";
  }
  void write(const uint8_t *b, size_t n) { out.append((const char *)b, n); }
};
extern MockSerial Serial;
