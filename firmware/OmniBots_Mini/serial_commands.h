// =====================================================================================
//  serial_commands.h - driving by commands typed in the Serial Monitor (over USB)
// =====================================================================================
//
//  Open the Serial Monitor (115200 baud, line ending "New Line"), type help and press
//  Enter. docs/05_First_Drive.md gives the full list of commands.
//
//  serial_commands.cpp applies these safety rules:
//    - nothing runs until Enter is pressed. A line that arrives without a line ending
//      stops the robot after a short time, and never runs
//    - any typed text that is not a valid command stops the robot
//    - the Serial Monitor always takes priority over the phone: a stop or a move typed
//      here switches the phone's Joystick mode off (otherwise the phone would start the
//      robot again)
//
// =====================================================================================

#pragma once

// Handle any typed text. Call this from loop().
void readSerialCommands();

// End a typed move when its time is up. Call every 10 ms.
void updateTimedMove(unsigned long now);

// Stop the robot, say so in the Serial Monitor, and switch the phone's Joystick mode off.
void serialStop();

// Cancel a typed move without printing anything (the phone has taken over).
void cancelTimedMove();

// True once anything has been typed. Until then the robot prints nothing.
bool someoneTyped();
