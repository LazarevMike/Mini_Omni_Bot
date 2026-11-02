# Lesson 01: Spin one motor

Upload a first sketch that turns the M1 wheel forward, stops it, turns it backward and stops it
again.

**Objectives**

- Upload a sketch to the Romeo ESP32-S3 with Arduino IDE 2.
- Explain how the PH and EN pins set a motor's direction and power.
- Describe how pulse-width modulation (PWM) produces any power between off and full.

## You will need

Mini Omni Bot, wired as in [Wiring](../../../docs/03_Wiring.md), a charged battery, a USB-C
data cable, and a mug or small box to hold the wheels off the ground.

## Wiring

The lesson uses the M1 motor in socket M1 and the PMODE, M1EN, M1PH and JP1 jumpers
([Wiring, step 2](../../../docs/03_Wiring.md#2-check-the-jumpers)). The motor wires may be
either way round; lesson 03 sets the direction.

## The code

The **sketch**, the Arduino program for this lesson, is [01_Spin_One_Motor.ino](01_Spin_One_Motor.ino).

### Key lines explained

```cpp
const int M1_EN_PIN = 12;  // EN = speed. Switched on and off very fast (PWM). 0 = brake.
const int M1_PH_PIN = 13;  // PH = direction. HIGH = forward, LOW = backward.
const int SPEED = 80;
```

PH (phase) selects the direction and EN (enable) sets how hard the motor is driven. `SPEED` is a
power level from 0 (off) to 255 (full); 80 is about 30%.

```cpp
  digitalWrite(M1_PH_PIN, HIGH);  // PH HIGH: forward
  analogWrite(M1_EN_PIN, SPEED);  // EN pulsing: run at this power
  delay(2000);
```

`digitalWrite` sets a pin to 3.3 V (`HIGH`) or 0 V (`LOW`). `analogWrite` switches the pin on and
off rapidly, with `SPEED` setting the share of time it is on. `delay(2000)` pauses the program
for 2000 ms while the motor keeps running. `analogWrite(M1_EN_PIN, 0)` then brakes the motor, and
`loop()` repeats the cycle indefinitely.

## Upload and run

[Software setup](../../../docs/04_Software_Setup.md), sections 2 to 6, installs the Arduino
IDE 2 and uploads this sketch. If it is running, continue with
[What you will see](#what-you-will-see). Later lessons upload in the same way:

1. Rest the robot on the mug or box. Switch SW off, connect the battery, and switch SW on.
2. Open `01_Spin_One_Motor.ino`, check the Tools settings
   ([Software setup, section 4](../../../docs/04_Software_Setup.md#4-select-the-board-settings-and-port))
   and click **Upload**.
3. Open the Serial Monitor at 115200 baud with **New Line**.

## What you will see

About 3 s after the upload, the Serial Monitor shows:

```text
Lesson 01: forward 2 s, stop 1 s, backward 2 s, stop 1 s, and again.
Forward
Stop
Backward
Stop
Forward
...
```

The M1 wheel turns one way for 2 s, stops for 1 s, turns the other way for 2 s and stops again.
A faint whine is normal: `analogWrite` switches the motor 1000 times per second.

## How it works

The Romeo's motor drivers each contain an **H-bridge**, four switches that connect the battery
to the motor in either direction. With PMODE fitted, each driver follows two pins:

| EN | PH | Motor |
|----|----|-------|
| LOW | either | brakes: both terminals are joined, so the wheel stops sharply |
| HIGH | HIGH | turns forward |
| HIGH | LOW | turns in reverse |

**Pulse-width modulation (PWM)** switches EN on and off at a fixed rate; the share of each
cycle for which it is on, the **duty cycle**, sets the power: an `analogWrite` value of 64 gives
25%, 128 gives 50% and 255 gives 100%.

> [!NOTE]
> **Key idea:** PH selects the direction and EN sets the power. Switching EN with PWM gives any
> power between off and full.

[How it works, section 4](../../../docs/07_How_It_Works.md#4-motor-drivers-h-bridges-and-pwm)
explains H-bridges and PWM in full.

## If it doesn't work

| Symptom | Likely cause | Fix |
|---------|--------------|-----|
| Text appears, but the wheel does not move | No motor power, or a missing jumper | Check the battery on VM and VIN, SW on, the PMODE, M1EN and M1PH jumpers, and the motor in M1. |
| The Serial Monitor stays empty | USB CDC On Boot disabled, or the wrong port | Set **USB CDC On Boot** to **Enabled**, check the port, upload again and press RST. |
| The upload fails, or no port appears | SW off, a charge-only cable, or the port in use | Switch SW on, close the Serial Monitor, try another cable ([Software setup](../../../docs/04_Software_Setup.md#if-it-doesnt-work)). |

## Summary

- **Upload** builds and sends a sketch; its messages appear in the Serial Monitor at 115200 baud.
- Each motor driver takes two signals: PH for direction and EN for power.
- `analogWrite` produces PWM; its value (0 to 255) sets the duty cycle and therefore the power.

**Next:** [Lesson 02: Count wheel turns](../02_Count_Wheel_Turns/README.md)
