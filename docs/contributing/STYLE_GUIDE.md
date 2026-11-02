# Style guide

This guide sets the voice, page structure and conventions for all Mini Omni Bot documentation
and code comments.

## Voice

- Write as a clear, modern engineering textbook for school and university beginners: precise,
  neutral, complete sentences, with no slang, anecdotes, jokes, exclamation marks, rhetorical
  questions, emoji or promotional words.
- Give instructions in the imperative ("Connect the motor cable to M1.") and explanations in
  the present tense, third person ("The encoder produces two square waves.").
- Define each technical term in **bold** at its first use on a page, in one sentence.
- Use tables for reference facts and numbered lists for procedures. Link instead of repeating.
- Quote robot output exactly as printed, even where its wording differs from these rules: the
  full program prints "web joystick" for the driving page.
- Mark a fact taken from DFRobot's schematic but not yet checked on the robot with "(from
  DFRobot's schematic)" at its first use on a page.
- Write "to be confirmed" only where a reader must know that a value is unsettled. Leave out any
  other unknown. Pages carry no TODO comments.
- Code comments follow the same voice: a short header per file, comments that explain why
  rather than what, and lines under about 100 characters.

## Spelling and units

Use Australian spelling (colour, centre, metre, licence as a noun, license as a verb). Put a
space between number and unit (12 V, 350 rpm), but none before degrees and percent (90°, 30%).
Write pins as printed on the board: IO4, M1, VM, 3V3, GND.

## Vocabulary

| Use | Avoid | Notes |
|-----|-------|-------|
| anticlockwise, clockwise | counter-clockwise | Seen from above |
| slide left, slide right | strafe | The `a` and `d` commands |
| turn left, turn right | rotate, spin | The `q` and `e` commands |
| Serial Monitor | console | The robot's text window over USB |
| terminal | command window, shell | The command line opened by PlatformIO's **New Terminal** |
| driving page | web UI, phone page | The page served to the phone |
| phone joystick | web remote | The feature, set by `REMOTE_CONTROL` in `config.h` |
| Tested, Builds, Untested | supported, compatible | Board status ([Other boards](../08_Other_Boards.md)) |
| Mini Omni Bot | the Mini Omni Bot | A name takes no article |
| Omni Wheel by Omnia, Omni Wheels by Omnia | Omnia wheel | The product; "omni wheel" in lower case is the general term |
| Omnia Wheel | Omnia | The company |

## Page templates

A guide page:

```text
# Title
One sentence: what the page achieves.
**Objectives**: 2 to 4 bullets, each starting with a verb
## You will need       (hands-on pages only; a compact table)
## 1. ..., ## 2. ...   (each step: action, then expected result; at most three sentences)
## If it doesn't work  (Symptom | Likely cause | Fix; 2 or 3 rows)
## Summary             (3 to 5 bullets)
**Next:** a link to the next page
```

A lesson has the same opening and closing, with these sections between: You will need (a
compact table, or one sentence when the setup is that of the previous lesson), Wiring, The code
(a sketch link, then Key lines explained), Upload and run, What you will see (exact output in a
`text` block), How it works, If it doesn't work. No page has quiz, exercise or "Try this"
sections. Index and reference pages (the READMEs, the parts list and this guide) have no
Objectives or Summary.

## Callouts

Use GitHub alerts, at most three per page, placed before the step they concern:

| Alert | Use |
|-------|-----|
| `> [!NOTE]` | A key idea; start with **Key idea:** and a lower-case word |
| `> [!TIP]` | An optional shortcut |
| `> [!WARNING]` | A risk to the hardware |
| `> [!CAUTION]` | A risk to people: a moving robot, the battery |

## Pictures

Add a picture only when it shows what words cannot. Until its file exists, a picture stays
hidden as a comment:

```text
<!-- ![The motor cable in M1, top view](images/wiring/motor_in_m1.jpg) -->
```

List each picture still to make in the [picture list](PICTURES.md), then run
`python3 tools/show_pictures.py`, which shows every picture whose file exists. To remove a
picture, delete its line. Videos are not used.

Photos are of the real robot. The full-size originals are not in the repository; they are kept
locally in `docs/images/originals/`, which git ignores. `tools/process_photos.py` calibrates them
into `docs/images/photos/`, adjusting only colour, light and bare desk. `tools/label_photos.py`
adds labels and writes to `docs/images/parts/`, `assembly/` and `wiring/`, and needs only the
processed photos.

## Names in the program

The firmware folder and sketch (`firmware/OmniBots_Mini`), the hotspot (`OmniBots_Mini`) and the
title of the driving page keep the name in the code. Write them exactly as the program uses them,
and use Mini Omni Bot everywhere else.
