# Assembly

This page builds Mini Omni Bot from its parts: the motors and wheels on the lower plate, the
board above them and the top plate over everything, with the wiring done part-way through.

**Objectives**

- Identify the printed parts, and the name and position of each motor.
- Fit the motors, wheels, board frame and board in order.
- Close the robot after wiring, with the battery holder, pillars, antenna and top plate.

## You will need

![The parts of one robot laid out on a table, each with a label](images/parts/all_parts.jpg)

The picture leaves out the pillars, the screws and nuts, and the battery.

| Item | Qty | Description |
|------|-----|-------------|
| **Motor plate** | 1 | The lower perforated hexagon plate; three raised **channels** on its underside hold the motors |
| **Top plate** | 1 | The upper perforated hexagon plate |
| **Board frame** | 1 | A flat frame with a square window and four corner pegs, on which the board sits |
| **Battery holder** | 1 | A shallow tray with two long rails that holds the battery |
| **Pillar** | 3 | A hexagonal post that holds the top plate above the motor plate |
| **Antenna bracket** | 1 | A small upright stand that carries the flat WiFi antenna |
| **Hub** | 3 | The part that joins a wheel to a motor shaft |
| DFRobot FIT0493 motor, with its six-wire cable | 3 | |
| Omni Wheel by Omnia | 3 | |
| DFRobot Romeo ESP32-S3 board, with its WiFi antenna | 1 | |
| Single-way female **Dupont** lead, with heat-shrink sleeve | 12 | A Dupont lead ends in a small socket that pushes onto a header pin |
| Battery lead with an **XT60** plug | 1 | XT60 is a high-current battery connector |
| Battery | 1 | Not yet selected; see the [parts list](../hardware/BOM.md#power) |
| **Button-head screws** and hex nuts | See the [parts list](../hardware/BOM.md#printed-parts-and-fasteners) | A button-head screw has a low, rounded head turned with a hex key |
| Hex key, small cross-head or flat screwdriver, masking tape | | |

The printed parts are described in [hardware/](../hardware/README.md), and the
[parts list](../hardware/BOM.md) gives the complete list.

## Wheel layout

![Top view of the robot: M1 at 60°, M3 at 180° and M4 at 300°](images/drawings/wheel_angles_top_view.svg)

| Motor | Position | Angle |
|-------|----------|-------|
| M1 | front left | 60° |
| M3 | back | 180° |
| M4 | front right | 300° |

> [!NOTE]
> **Key idea:** each motor is named after the board socket that drives it, and its angle is
> measured anticlockwise from the front, seen from above.

The three wheel positions are alike, so the wiring decides the front: the wheel connected to M3
is the back of the robot, and the front is the side between M1 and M4.

## Before you start

1. Remove all support material, especially from screw holes and channels.
2. Prepare each motor cable. Its red and white wires go straight to the board; join a Dupont lead
   to each of its other four wires and cover each joint with heat-shrink sleeve, as on the cables
   in the parts picture. The method of joining is to be confirmed.
3. Work through this page in order. Step 7 sends you to [Wiring](03_Wiring.md) and back, because
   the top plate later covers the board's terminals.

> [!WARNING]
> Threads in printed plastic strip easily. Tighten each screw only until the part stops moving.

## 1. Fit the motors

![Three motors lying in the channels on the underside of the motor plate, their cables fanned out; the motor, shaft, channel, cable plug, encoder and a Dupont connector are labelled](images/assembly/motors_in_channels.jpg)

Turn the motor plate underside up and lay a motor in each channel, with its shaft pointing out
through the end wall. Fix each motor with two short screws through the end wall, one on either
side of the shaft; a longer screw can damage the gearbox. Push
a cable's plug into the socket on each **encoder**, the sensor board at the rear of the motor;
each shaft now points straight out past the edge of the plate.

## 2. Fit the hubs into the wheels

![A wheel beside a separate hub, labelled: the rollers and body of the wheel, and the hub's hexagonal body and narrower round stem](images/assembly/hub_and_wheel.jpg)

![A wheel with its hub pushed into the centre, the hub's stem and bore facing the camera, with the parts labelled](images/assembly/hub_in_wheel.jpg)

Push the hexagonal body of a hub into the centre of each wheel, with the stem and its bore on
the side that will face the motor. The hub now sits square in the wheel and does not turn
separately from it.

## 3. Fit the wheels

![The top side of the motor plate with the wheels beside it: the lower wheel is off, and its bare shaft and two motor screw heads, labelled, show at the plate's edge](images/assembly/wheel_going_on.jpg)

Turn the plate top side up, and push each hub onto its motor shaft, leaving a small gap between
the wheel and the channel's end wall. Each wheel now turns freely, without wobbling or rubbing.

## 4. Fix the board frame

![The board frame laid on the top side of the motor plate, with the centre slot inside its window; screws and nuts lie beside the plate, and each part is labelled](images/assembly/frame_on_plate.jpg)

![Close-up of the fixed frame: a peg and a button-head screw at each corner, and the slot inside the window, with the parts labelled](images/assembly/frame_close_up.jpg)

Lay the frame on the top side of the motor plate, pegs upwards and off-centre, so that the plate's
oblong centre slot lies inside the window, along one side of it. Fix it with button-head screws
through the holes beside the pegs, with a nut on the underside of each; the finished robot uses
two, at diagonally opposite corners, and the close-up shows all four holes used. The frame now
lies flat and does not shift.

## 5. Pass the cables through the slot

![The three motor cables coming up through the centre slot in separate bundles, with the frame, a peg and the two frame screws labelled](images/assembly/cables_through_slot.jpg)

Choose any wheel as M3; seen from the top side, the next wheel anticlockwise is M4 and the next is
M1, as in the [wheel layout](#wheel-layout). Wrap masking tape round each motor's six wires near
their ends and write the motor's name on it, because the wires of the three cables look alike at
the board. Pass the cables up through the slot from the underside, in three separate bundles. All
the loose ends now lie on the top side.

## 6. Fit the board

![The Romeo board on the frame, close up: a peg tip in a gold-ringed corner hole, a frame screw beside another corner, and the USB-C port, each labelled](images/assembly/board_on_pegs.jpg)

Check that no screws or metal fragments lie on the frame, because they can short-circuit the
board. With the edge that carries the M1 and M3 terminals towards the free half of the plate,
where the battery holder goes in [step 8](#8-fit-the-battery-holder), lower the board so that the
four gold-ringed holes at its corners pass over the four pegs, keeping the wires off the pegs; it
takes no screws of its own. The board now rests level on the pegs, and the wires shown already in its terminals are connected in the next step.

## 7. Wire the board

![Encoder leads pushed onto the board's encoder header in three layers, with one column free; the Dupont connectors, heat-shrink sleeve, header and M1 terminal are labelled](images/assembly/encoder_leads.jpg)

Follow [Wiring](03_Wiring.md) now, while the terminals and the header are open. Switch **SW**,
the board's power switch, off and disconnect the battery. Every motor wire, encoder lead and
battery-lead wire is now connected.

## 8. Fit the battery holder

> [!CAUTION]
> If the battery is lithium polymer (LiPo), damage can make it catch fire. Keep sharp edges away
> from it, do not press it hard enough to dent it, and make sure it cannot slide into a wheel.

![The whole robot from above before the top plate: the battery holder beside the board's M1 and M3 terminals, with loose wires and the XT60 plug resting in it, and the USB-C port on the next edge of the board, clear of the holder](images/assembly/holder_beside_board.jpg)

Fix the holder on the top side of the motor plate, beside the board's M1 and M3 terminals, with a
nut on the underside of each screw; the finished robot uses two screws, at diagonally opposite
corners.
Hold the plate on edge, with one hand on the board, while you fit the nuts. The USB-C port is
on the next edge of the board, clear of the holder, so it stays reachable for
[Software setup](04_Software_Setup.md); keep the loose wires away from it.

Lift the XT60 plug and the loose wires out of the holder, where the picture shows them resting.
Fit the battery, with its power leads within reach. Keep its **balance lead**, the small lead used
by the charger, within reach too. Do not connect the battery yet. The holder now sits flat, and the battery cannot move towards a wheel.

## 9. Fit the pillars

![Three pillars, each labelled, standing on the motor plate around the board, with the wiring between them and the holder in an earlier position, on the USB-C edge](images/assembly/pillars_on_plate.jpg)

Keep the holder where step 8 places it; in this picture it is on the board's USB-C edge. Stand the
three pillars on the top side of the motor plate, at the corners labelled, where they meet the
three screws of the top plate shown in step 11. Holding the plate on edge, with one hand on the
board, fix each with a button-head screw from the underside. Gather the wires inwards, between
the pillars. Each pillar now stands upright and does not turn.

## 10. Fit the antenna

![The top plate held above the robot, with the antenna on its bracket and the thin lead passing down through a plate hole to the socket on the board, with the parts labelled](images/assembly/antenna_on_plate.jpg)

![The bracket on the top plate, held by two labelled nuts, with the lead entering a hole beside it](images/assembly/bracket_nuts.jpg)

![The antenna plug pressed onto the socket at the corner of the module, with the plug, lead and module labelled](images/wiring/antenna_clipped_on.jpg)

The board has no built-in antenna, so this antenna is the robot's only radio link, and it goes
high and near an edge, away from the motors, the battery and metal, which block the signal.

1. Stick the flat antenna to the bracket's upright face.
2. Fix the bracket near an edge of the top plate, with two screws and a nut on top of each.
3. Pass the lead down through a plate hole beside the bracket.
4. Hold the plate above the robot and find the **U.FL socket**, a miniature snap-on radio
   connector, at the corner of the ESP32-S3 module.
5. Hold the plug directly above the socket and press straight down until it clicks and sits flat.

The lead reaches the socket without strain. To remove the plug, lift it straight up with a
fingernail; never pull the lead.

## 11. Fit the top plate

![The finished robot from above: the top plate with its three screws and the antenna bracket at one edge, labelled, and the three wheels showing around it](images/assembly/finished_top.jpg)

![The finished robot from below: three motors in their channels, the cable plugs, the nuts and the three wheels, with one of each labelled](images/assembly/finished_underside.jpg)

Remove the tape from the wires. Lower the top plate onto the pillars, with its slot above the
motor plate's slot and every wire between the plates, and fix it with three button-head screws
from above. Turn each wheel a full turn by hand; it touches no cable. The robot now looks like the
pictures, from above and from below.

## If it doesn't work

| Symptom | Likely cause | Fix |
|---------|--------------|-----|
| A wheel rubs on its channel | Wheel too far onto the shaft | Move the wheel out slightly. |
| A wheel wobbles or slips | Hub not square in the wheel, or not fully on the shaft | Seat the hub square and push it fully on. |
| A wheel touches a wire | A wire loop lies outside the plates | Draw the loop back between the plates. |

## Summary

- Each motor is named after its board socket, and its position is an angle from the front.
- The motors lie beneath the motor plate; the board and the battery holder lie on top of it, and
  the top plate carries the antenna.
- The wiring comes after the board is on its pegs and before the top plate goes on.
- Screws into printed parts are tightened only until the part stops moving.
- The battery sits low in its holder and is not yet connected, and every wire is clear of the
  wheels.

**Next:** [Software setup](04_Software_Setup.md): install the tools and upload a first program.
