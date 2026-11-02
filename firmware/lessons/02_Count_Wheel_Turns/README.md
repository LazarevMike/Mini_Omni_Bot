# Lesson 02: Count wheel turns

Read the M1 motor's encoder and display how far the wheel has turned, in either direction.

**Objectives**

- Read a motor's encoder through two input pins.
- Explain how two offset signals give both distance and direction.
- Use interrupts to count every signal change.

## You will need

The setup from [lesson 01](../01_Spin_One_Motor/README.md), without the battery: the motor stays
off, and the encoder runs from 3V3, which USB supplies. Keep SW on.

## Wiring

The encoders are connected in [Wiring, step 4](../../../docs/03_Wiring.md#4-connect-the-encoders).
This lesson reads only M1's **encoder**, a sensor that produces pulses as a shaft turns: Enc A on
IO4 and Enc B on IO5, powered from 3V3.

## The code

The sketch is [02_Count_Wheel_Turns.ino](02_Count_Wheel_Turns.ino).

### Key lines explained

```cpp
volatile long m1Count = 0;

void IRAM_ATTR m1EncoderAChanged() {
  if (digitalRead(M1_ENCODER_A_PIN) != digitalRead(M1_ENCODER_B_PIN)) {
    m1Count = m1Count + 1;
  } else {
    m1Count = m1Count - 1;
  }
}

attachInterrupt(digitalPinToInterrupt(M1_ENCODER_A_PIN), m1EncoderAChanged, CHANGE);
```

An **interrupt** makes the processor pause its current task, run a short function, and then
resume. `attachInterrupt` runs `m1EncoderAChanged` whenever pin A rises or falls (`CHANGE`). If A
and B then differ, the wheel has moved one step forward; if they are equal, one step back.
`m1EncoderBChanged` applies the opposite rule when B changes.

`volatile` tells the compiler that `m1Count` can change at any moment, so every read fetches
its current value, and `IRAM_ATTR` places the function in the ESP32's fast internal memory.
`loop()` copies the count with interrupts briefly paused, so that it is read in one piece, and
prints it every 200 ms.

## Upload and run

1. Connect USB and switch SW on. Do not connect the battery.
2. Upload `02_Count_Wheel_Turns.ino` and open the Serial Monitor (115200 baud), as in
   [lesson 01](../01_Spin_One_Motor/README.md#upload-and-run).
3. Turn the M1 wheel slowly by hand, in both directions.

## What you will see

Five lines per second. With the wheel still, and then while it turns (values depend on how far):

```text
M1: 0 counts   0.00 turns   A=1 B=1
M1: 313 counts   0.21 turns   A=0 B=1
M1: 750 counts   0.50 turns   A=0 B=0
M1: 1496 counts   1.00 turns   A=1 B=1
```

The count rises in one direction and falls, through 0 to negative values, in the other. One
wheel turn is about 1496 counts. `A=` and `B=` show the present state of each signal. Which
direction counts up depends on the wiring; lesson 03 accounts for it.

## How it works

A magnet disc on the motor shaft passes two magnetic sensors, A and B, each producing 11 pulses
per motor turn. The sensors are a quarter of a pulse apart, an arrangement called
**quadrature**: in one direction A changes first, in the other B changes first. One forward pulse
gives four counts:

```text
A        0   1     1     0     0
B        0   0     1     1     0
Changed      A     B     A     B
A and B      diff  same  diff  same
Count    0   1     2     3     4
```

In reverse the same states occur in the opposite order, and each step counts down. At full
speed this is about 8700 changes per second, far more than `loop()` could check while it prints
and waits; interrupts catch every one.
[How it works, section 5](../../../docs/07_How_It_Works.md#5-counting-wheel-turns-encoders)
derives the 1496 counts per wheel turn.

> [!NOTE]
> **Key idea:** two signals a quarter of a pulse apart give both distance (the number of
> changes) and direction (which signal changes first).

## If it doesn't work

| Symptom | Likely cause | Fix |
|---------|--------------|-----|
| The count stays at 0; A and B never change | Encoder not powered or not connected | Check Enc A on IO4, Enc B on IO5, Enc + on 3V3, Enc GND on GND. |
| The count alternates between two values | One signal wire not connected | The A or B value that never changes identifies the wire. |
| No text after the encoder is wired | An encoder wire on a strapping pin (IO0, IO3, IO45, IO46) | Move the wire to IO4 or IO5 and press RST. |

## Summary

- An encoder's two signals, A and B, are a quarter of a pulse apart (quadrature).
- Counting every change of both signals gives 1496 counts per wheel turn.
- Interrupts count changes as they occur, independently of `loop()`.

**Next:** [Lesson 03: Find your wheels](../03_Find_Your_Wheels/README.md)
