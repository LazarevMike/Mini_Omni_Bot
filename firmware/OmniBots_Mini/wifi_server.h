// =====================================================================================
//  wifi_server.h - WiFi and the driving page (part of the phone joystick)
// =====================================================================================
//
//  With the phone joystick off (REMOTE_CONTROL 0 in config.h, or a board that is not an
//  ESP32) these functions do nothing and `wifi` prints the reason, so the rest of the
//  program needs no special cases.
//
// =====================================================================================

#pragma once

// Start WiFi (the robot's hotspot, or a home network) and the web server.
void setupWifi();

// Pass messages from the driving page to phone_control.cpp and send it the robot's status.
// Call this from loop().
void updateWifi();

// The typed `wifi` command (args is the text typed after the word "wifi").
void wifiCommand(const char *args);
