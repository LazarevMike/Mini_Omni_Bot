# Mini Omni Bot guides

This index lists every guide and lesson in the order in which they are used. Each page ends
with a link to the next one; Wiring is done in the middle of Assembly, at its step 7.

The guides are intended for learners aged about 12 and over with a parent or teacher, and for
adults new to Arduino. Building, wiring, programming and driving take about 3 hours, and each
lesson 20 to 30 minutes.

## Guides

1. [Getting started](01_Getting_Started.md): the robot, its specification, the parts, safety
   and the glossary.
2. [Assembly](02_Assembly.md): building the robot; step 7 sends you to Wiring and back.
3. [Wiring](03_Wiring.md): the motors, encoders, jumpers and battery, during Assembly.
4. [Software setup](04_Software_Setup.md): Arduino IDE 2 or PlatformIO, and the first upload.
5. [Lessons 01 to 05](../firmware/lessons/README.md): the program, one idea at a time (below).
6. [First drive](05_First_Drive.md): the full program, typed commands, the motor test and
   calibration.
7. [Phone joystick](06_Phone_Joystick.md): driving from a phone over the robot's WiFi hotspot.
8. [How it works](07_How_It_Works.md): omni wheels, the wheel formula, motor drivers, PWM and
   encoders.
9. [Other boards](08_Other_Boards.md): other controllers with separate motor drivers.

## Lessons

The lessons come before First drive because lesson 03 produces the wheel settings that the full
program needs.

1. [Spin one motor](../firmware/lessons/01_Spin_One_Motor/README.md): direction and power
   control of one motor.
2. [Count wheel turns](../firmware/lessons/02_Count_Wheel_Turns/README.md): reading an
   encoder with interrupts.
3. [Find your wheels](../firmware/lessons/03_Find_Your_Wheels/README.md): the angle and
   direction settings of each wheel.
4. [Drive any direction](../firmware/lessons/04_Drive_Any_Direction/README.md): one formula
   for every direction of travel.
5. [Keyboard driving](../firmware/lessons/05_Keyboard_Driving/README.md): driving with keys
   typed in the Serial Monitor.

Technical terms are defined at first use and collected in the
[glossary](01_Getting_Started.md#glossary).

## Quick start for experienced makers

Connect the motors to M1, M3 and M4 and the encoders to IO4/IO5, IO6/IO7 and IO15/IO16, powered
from 3V3 (not 5V). Fit the PMODE, M1EN, M1PH and JP1 jumpers and connect the 12 V battery to VM
and VIN through a split lead ([Wiring](03_Wiring.md)). Upload with `pio run -t upload`, or with
Arduino IDE 2, esp32 boards 2.0.17, board **DFRobot Romeo ESP32-S3** and **USB CDC On Boot:
Enabled** ([Software setup](04_Software_Setup.md)). In the Serial Monitor (115200 baud, **New
Line**) type `help`; run `test` with the wheels raised, then calibrate `WHEEL_SETUP` in
`config.h` ([First drive](05_First_Drive.md)). For the phone joystick, type `wifi` for the
hotspot password, join `OmniBots_Mini` and open http://192.168.4.1
([Phone joystick](06_Phone_Joystick.md)).

The program also builds for other ESP32, Arduino and STM32 boards with separate motor drivers;
so far only the Romeo has driven the robot ([Other boards](08_Other_Boards.md)).

## Repository layout

| Path | Contents |
|------|----------|
| `docs/` | These guides and their pictures |
| `docs/contributing/` | [Style guide](contributing/STYLE_GUIDE.md) and [picture list](contributing/PICTURES.md), for writers |
| `firmware/lessons/` | Five lessons, each a sketch with a README |
| `firmware/OmniBots_Mini/` | The full program; settings in `config.h`, pins in `boards.h` |
| `hardware/` | [Parts list](../hardware/BOM.md); 3D models and drawings to come |
| `web/` | The driving page served to the phone |
| `tools/` | [Build checks, a host test, a simulated robot and the picture scripts](../tools/README.md) |

## Licence

Copyright 2026 Omnia Wheel. The guides and pictures in this folder, and the READMEs outside
`hardware/`, are licensed under [CC BY 4.0](LICENSE). The
[project README](../README.md#licences) lists the licence of every folder.
