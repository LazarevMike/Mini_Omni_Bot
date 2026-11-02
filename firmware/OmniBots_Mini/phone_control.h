// =====================================================================================
//  phone_control.h - turning the messages of the driving page into wheel movements
// =====================================================================================
//
//  The driving page (web/) sends its whole driving state (knob position, held turn,
//  Spin setting, speed limit) 10 to 20 times a second. This file decides what the
//  robot does with it, and applies these safety rules:
//
//    - only one phone drives at a time; any others are told that the robot is busy
//    - if the phone sends nothing for half a second (WiFi dropped, screen locked, tab
//      closed), the robot stops and clears any Spin
//    - a phone that sends nothing for 6 seconds loses control, so another can take over
//    - anything typed in the Serial Monitor takes priority (see serial_commands.h)
//
//  wifi_server.cpp handles the WiFi and the web server, and passes every message to the
//  functions here, one at a time.
//
//  Part of the phone joystick. When it is off (USE_PHONE_JOYSTICK 0, see boards.h), only
//  updatePhoneDriving() and phoneStandDown() remain, and they do nothing.
//
// =====================================================================================

#pragma once
#include <stddef.h>
#include <stdint.h>

// A driving page connected. Returns true if it receives control (no other page has it).
bool phoneConnected(uint32_t client, uint32_t now);

// A driving page disconnected.
void phoneDisconnected(uint32_t client);

// One message from a driving page, with its arrival time. Returns a reply to send back,
// or nullptr.
const char *phoneMessage(uint32_t client, const char *message, uint32_t receivedMs, uint32_t now);

// Some phone messages were lost on arrival: stop until the next full one arrives.
void phoneMessagesLost();

// Which page is in control (0 = none).
uint32_t phoneInControl();

// If the page in control has sent nothing for too long, release it. Returns that page
// (so that it can be disconnected), or 0.
uint32_t phoneGoneQuiet(uint32_t now);

// The status line the robot sends the page ten times a second.
size_t phoneStatus(char *text, size_t size, bool hotspot, int rssi);

// Turn the phone's latest instructions into wheel targets. Call every 10 ms.
void updatePhoneDriving(uint32_t now);

// A command was typed in the Serial Monitor: switch the phone's Joystick mode off.
void phoneStandDown();
