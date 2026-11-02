# Lessons

Five short programs introduce, one idea at a time, the code that drives Mini Omni Bot. They
have been compiled but not yet run on the robot, so the outputs shown are examples.

## The path

Each lesson folder holds a **sketch**, an Arduino program (`.ino`), and a `README.md` that
explains it. Take the lessons after [Software setup](../../docs/04_Software_Setup.md) and before
[First drive](../../docs/05_First_Drive.md); each takes 20 to 30 minutes.

| # | Lesson | What you learn |
|---|--------|----------------|
| 01 | [Spin one motor](01_Spin_One_Motor/README.md) | Control a motor's direction and power. |
| 02 | [Count wheel turns](02_Count_Wheel_Turns/README.md) | Read an encoder with interrupts. |
| 03 | [Find your wheels](03_Find_Your_Wheels/README.md) | Record each wheel's angle, flipMotor and flipEncoder. |
| 04 | [Drive any direction](04_Drive_Any_Direction/README.md) | Drive in any direction, or turn, with one formula. |
| 05 | [Keyboard driving](05_Keyboard_Driving/README.md) | Drive with typed keys; end every move with a timer. |

## You will need

| Item | Notes |
|------|-------|
| Mini Omni Bot, assembled and wired | [Assembly](../../docs/02_Assembly.md), [Wiring](../../docs/03_Wiring.md) |
| A charged 12 V battery | For example a 3S LiPo (exact pack to be confirmed); USB powers the board but not the motors |
| A computer with Arduino IDE 2 and a USB-C data cable | [Software setup](../../docs/04_Software_Setup.md) |
| A mug or small box, and about 1 m of clear floor | To hold the wheels off the ground |

The sketches are written for the DFRobot Romeo ESP32-S3; for other boards, see
[Other boards](../../docs/08_Other_Boards.md).

> [!CAUTION]
> Keep the wheels in the air, on the mug or box, unless a lesson places the robot on the floor.
> To stop it at once, lift it and switch SW off. Disconnect the battery before touching any
> wiring.

**Next:** [Lesson 01: Spin one motor](01_Spin_One_Motor/README.md)
