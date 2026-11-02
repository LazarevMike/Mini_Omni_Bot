// =====================================================================================
//  drive.h - the omni mathematics: a direction becomes a power for each wheel
// =====================================================================================
//
//  forward  how fast to go forward (negative = backward)
//  left     how fast to go left (negative = right)
//           Together they set the direction. Their combined size is the share of top
//           speed, from 0 to 1: the busiest wheel gets exactly that much power.
//  turn     extra power for turning, + = anticlockwise (left), - = clockwise
//
//  Examples: driveRobot(1, 0, 0) goes straight forward at full speed,
//            driveRobot(0, 0.3, 0) slides left at 30%,
//            driveRobot(0, 0, 0.4) turns left on the spot.
//
//  docs/07_How_It_Works.md explains the mathematics with pictures.
//
// =====================================================================================

#pragma once

void driveRobot(float forward, float left, float turn);

// Stop immediately: every wheel brakes, with no gradual slowing.
void stopRobot();
