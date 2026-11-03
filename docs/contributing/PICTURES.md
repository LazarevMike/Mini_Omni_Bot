# Pictures

This page lists the pictures still to make and those already in place. Photos are of the real
robot. The concept diagrams are drawn by a script; drawings from the 3D models will follow.

Save a new picture under `docs/images/` with exactly the name listed (`hero.JPG` does not match
`hero.jpg`), then run `python3 tools/show_pictures.py`: the picture then appears on every page
that uses it. Until its file exists, a picture stays hidden, so that GitHub shows no broken
image.

## Still to make

| File (in `docs/images/`) | What to shoot or draw | Pages that use it |
|--------------------------|-----------------------|-------------------|
| `software/setup_arduino_serial_monitor.png` | **Example in place: retake on the robot.** The current file is a crop of the real Arduino IDE 2 with lesson 01's output added by hand, made without a board; docs 04 says so under the picture, and that line goes with the retake. Screenshot, Arduino IDE 2 with lesson 01 running on the robot: the Serial Monitor showing the "Lesson 01:" line, then Forward, Stop, Backward, Stop, with the "New Line" and "115200 baud" drop-downs circled. | docs 04 |
| `assembly/pillars_on_plate.jpg` | **Retake.** The source photo, IMG_6958, shows the battery holder on the USB-C edge, which is not its final position. Photo: the three pillars standing, with the holder on the M1/M3 side. Then delete the note about it in the step. | docs 02, step 9 |
| `assembly/hub_and_wheel.jpg` | **Retake.** The hub in the source photo, IMG_6944, is soft with a pale halo. Photo: a wheel beside a separate hub, both sharp, on a plain background. | docs 02, step 2 |
| `assembly/hub_in_wheel.jpg` | **Retake.** The source photo, IMG_6945, has a smear on the table under the roller. Photo: a wheel with its hub inserted, the stem and bore facing the camera. | docs 02, step 2 |
| `assembly/placement_top_view.svg` | Drawing from the 3D model: the top side of the motor plate with the exact holes for the board frame, the battery holder and the three pillars, and of the top plate for the antenna bracket. The photos show the positions only roughly. | docs 02, steps 4, 8, 9, 10 |
| `assembly/battery_in_holder.jpg` | Photo: the chosen battery in its holder, leads within reach, once the pack is selected. The present picture shows the holder empty. | docs 02, step 8 |
| `assembly/usb_c_access.jpg` | Photo: the USB-C cable in the board with the holder fitted. | docs 02, step 8; docs 04 |
| `assembly/cable_joint.jpg` | Photo, close-up: one encoder wire joined to a Dupont lead under heat-shrink sleeve, once the method of joining is settled. | docs 02, Before you start |
| `wiring/power_light_on.jpg` | Photo: the PWR light on with SW on. | docs 03, step 5 |

## Photos

The full-size originals are not in the repository. To process photos again, place them locally in
`docs/images/originals/`, which git ignores; without them, `process_photos.py` stops with a message
saying so. To move a label, only the second command is needed:

```text
python3 tools/process_photos.py          (every photo the docs use, or give numbers: 6934 6938)
python3 tools/label_photos.py            (parts/, assembly/, wiring/)
```

`process_photos.py` needs numpy, OpenCV and Pillow, and adjusts only colour, light and bare desk;
it paints nothing on the robot. Its `SETTINGS` table holds each photo's crop and masks. A number
on the file name is the camera's, so the order of the numbers is the order of the build.

`docs/images/photos/` holds only the photos the docs need: those shown without labels, IMG_6934,
IMG_6942 and IMG_6954 (README) and IMG_6961 (docs 01), and the sources of the labelled pictures below. IMG_6940, IMG_6947,
IMG_6951, IMG_6956 and IMG_6957 are on no page, so they are left out; `process_photos.py` makes them
only when asked for by number.

## Labelled pictures

Made by `tools/label_photos.py`. The file name says what each picture shows. No label names a wheel
M1, M3 or M4, because the photos cannot prove which is which.

| File (in `docs/images/`) | From photo | Pages that use it |
|--------------------------|------------|-------------------|
| `parts/all_parts.jpg` | IMG_6942 | docs 01; docs 02 |
| `assembly/motors_in_channels.jpg` | IMG_6943 | docs 02, step 1 |
| `assembly/hub_and_wheel.jpg` | IMG_6944 | docs 02, step 2 |
| `assembly/hub_in_wheel.jpg` | IMG_6945 | docs 02, step 2 |
| `assembly/wheel_going_on.jpg` | IMG_6946 | docs 02, step 3 |
| `assembly/frame_on_plate.jpg` | IMG_6948 | docs 02, step 4 |
| `assembly/frame_close_up.jpg` | IMG_6949 | docs 02, step 4 |
| `assembly/cables_through_slot.jpg` | IMG_6950 | docs 02, step 5 |
| `assembly/board_on_pegs.jpg` | IMG_6937 | docs 02, step 6 |
| `assembly/encoder_leads.jpg` | IMG_6954 | docs 02, step 7 |
| `assembly/holder_beside_board.jpg` | IMG_6936 | docs 02, step 8 |
| `assembly/pillars_on_plate.jpg` | IMG_6958 | docs 02, step 9 |
| `assembly/antenna_on_plate.jpg` | IMG_6959 | docs 02, step 10 |
| `assembly/bracket_nuts.jpg` | IMG_6960 | docs 02, step 10 |
| `assembly/finished_top.jpg` | IMG_6962 | docs 02, step 11 |
| `assembly/finished_underside.jpg` | IMG_6963 | docs 02, step 11 |
| `assembly/underside_motors.jpg` | IMG_6939 | docs 07 |
| `wiring/romeo_overview_labelled.jpg` | IMG_6938 | docs 03 |
| `wiring/jumpers_fitted.jpg` | IMG_6938 | docs 03, step 2 |
| `wiring/motor_terminals_m1_m3.jpg` | IMG_6952 | docs 03, step 3 |
| `wiring/motor_terminal_m4.jpg` | IMG_6937 | docs 03, step 3 |
| `wiring/encoder_layers.jpg` | IMG_6953 | docs 03, step 4 |
| `wiring/encoder_header.jpg` | IMG_6954 | docs 03, step 4 |
| `wiring/battery_lead.jpg` | IMG_6955 | docs 03, step 5 |
| `wiring/antenna_clipped_on.jpg` | IMG_6941 | docs 02, step 10 |

## Screenshots

| File (in `docs/images/`) | What it shows | Pages that use it |
|--------------------------|---------------|-------------------|
| `software/phone_page_tour.png` | The driving page on a phone, with callouts 1 to 10; made with `tools/fake_robot.py --hotspot` | README; docs 06 |
| `software/setup_arduino_serial_monitor.png` | Example, not a capture: see [Still to make](#still-to-make) | docs 04 |

## Product pictures

| File (in `docs/images/`) | What it shows | Pages that use it |
|--------------------------|---------------|-------------------|
| `parts/omni_wheels_by_omnia.png` | Omnia Wheel's product picture of its omni wheel range, transparent background; links to the shop | README |

## Drawings

Drawn by `tools/make_drawings.py`, with the same wheel angles as `config.h` and the board layouts
published by DFRobot and Pololu. To change one, edit the script and run `python3 tools/make_drawings.py`, then `python3 tools/show_pictures.py`.

| File (in `docs/images/`) | What it shows | Pages that use it |
|--------------------------|---------------|-------------------|
| `drawings/omni_wheel_directions.svg` | One omni wheel, face on: "rolls and pushes this way" along the floor, "slides this way, on its rollers" along the axle; beside it, the same wheel from above. A photo of an Omni Wheel by Omnia with arrows drawn on may replace it. | docs 01, docs 07 |
| `drawings/wheel_angles_top_view.svg` | Top view, front up: 0° front, 90° left, 180° back, 270° right, an anticlockwise arrow for the direction in which angles count, M1 at 60°, M3 at 180°, M4 at 300°. To be redrawn from the 3D model if the finished chassis differs. | docs 02, docs 05, lesson 03 |
| `drawings/kiwi_layout_top.svg` | The same top view, with an arrow at each wheel for the way it pushes on positive power: all three anticlockwise round the centre. | docs 07, lesson 04 |
| `drawings/move_shadow.svg` | The projection in three panels: a move along the wheel's rolling direction (all of it), at 60° (half) and at right angles (none). | docs 07 |
| `drawings/kiwi_forward.svg` | Going forward: M1 (-1) pushing forward and slightly right, M4 (+1) forward and slightly left, M3 (0) sliding; each push split into the forward part and the sideways part that cancels. | docs 07 |
| `drawings/kiwi_slide_left.svg` | Sliding left: M3 (-1) pushing left, M1 (+0.5) back and slightly left, M4 (+0.5) forward and slightly left, the forward and back parts crossed out. | docs 07 |
| `drawings/drive_commands_top_view.svg` | Top view with an arrow for each move: `w`, `s`, `a`, `d`, `go 45`, and two small robots with a curved arrow for `q` (anticlockwise) and `e` (clockwise). | docs 05, lesson 05 |
| `drawings/h_bridge.svg` | The H-bridge's four states: forward (EN 1, PH 1), reverse (EN 1, PH 0), brake (EN 0) and coast, with the path of the motor current. | docs 07 |
| `drawings/pwm_duty.svg` | Three signals with duty cycles of 25%, 50% and 75%, their `analogWrite` values and average power. | docs 07 |
| `drawings/wiring_diagram.svg` | The Romeo as in DFRobot's On-board Function Diagram, with the three motors, the battery on VM and VIN, and an enlarged encoder header. Wire colours are those of the real cables. | docs 03 |
| `drawings/encoder_ab.svg` | Signals A and B a quarter cycle apart, every change counted: +1 when A changes first, −1 when B changes first. | docs 07 |
| `drawings/push_direction.svg` | One wheel from above and from outside the robot: a top moving right pushes anticlockwise (flipMotor false), a top moving left clockwise (flipMotor true). | lesson 03 |
| `drawings/drv8876_one_motor.svg` | One Pololu #4036 board with its pins in their real order, wired to a controller, the battery through a switch, and a motor. | docs 08 |
