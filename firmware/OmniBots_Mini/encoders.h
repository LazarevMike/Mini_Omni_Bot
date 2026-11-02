// =====================================================================================
//  encoders.h - counting how far each wheel has turned
// =====================================================================================
//
//  Each motor has a small encoder on its rear with two outputs, A and B. As the wheel
//  turns they switch on and off, a quarter of a cycle apart. The output that changes first
//  gives the direction, and the number of changes gives the distance turned.
//  One full wheel turn is COUNTS_PER_WHEEL_TURN counts (config.h).
//
//  On boards where the encoder pins cannot use interrupts (USE_ENCODERS 0 in boards.h),
//  the counts stay at 0. The robot still drives.
//
// =====================================================================================

#pragma once

// Start counting the encoder signals.
void setupEncoders();

// The counts a wheel has turned since the start (or since resetEncoders()).
// Positive means the wheel turned in the direction that a positive power turns it.
long encoderCount(int wheel);

// Set every count back to 0.
void resetEncoders();

// The wheel's speed in turns per minute, measured over the last 100 ms.
float wheelRpm(int wheel);

// Calculate the wheel speeds. Call every 10 ms; it recalculates every 100 ms.
void updateWheelSpeeds(unsigned long now);

// The present level (0 or 1) of an encoder wire, used for checking the wiring.
int encoderPinLevel(int wheel, bool channelA);
