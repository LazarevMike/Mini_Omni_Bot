# Parts list (BOM)

This bill of materials lists every part for one Mini Omni Bot. Suppliers are examples only.
The battery has not been selected and the chassis files are not published yet.

## Electronics

| Qty | Part | Example source | Notes |
|-----|------|----------------|-------|
| 1 | DFRobot Romeo ESP32-S3 (DFR0994) | [DFRobot](https://www.dfrobot.com/product-2743.html), [wiki](https://wiki.dfrobot.com/dfr0994/) | ESP32-S3-WROOM-1U-N16R8 module and 4 × DRV8876 motor drivers on one board |
| 1 | 2.4 GHz WiFi antenna (U.FL plug) | Supplied with the board | The module has no antenna of its own. |
| 3 | DFRobot FIT0493 motor with hall encoder | [DFRobot](https://www.dfrobot.com/product-1462.html), [Core Electronics](https://core-electronics.com.au/12v-dc-motor-350rpm-w-encoder-12kg-cm.html) (Australia) | 12 V, 350 rpm, 34:1 gearbox, 6-wire cable |
| 12 | Single-way female Dupont leads, with heat-shrink sleeve | Any | Four per motor cable, joining its encoder wires to the encoder header; its two motor wires go straight to M1/M3/M4 |
| 1 | USB-C data cable | Any | Charge-only cables do not carry data. |

A board other than the Romeo also needs three Pololu DRV8876 driver boards
([#4036](https://www.pololu.com/product/4036)) and a switch or plug in the battery lead; see
[Other boards](../docs/08_Other_Boards.md).

## Wheels

| Qty | Part | Example source | Notes |
|-----|------|----------------|-------|
| 3 | Omni Wheels by Omnia | Available from Omnia Wheel | Each fits on a printed hub (below) |

## Power

Read [Getting started: Safety](../docs/01_Getting_Started.md#safety) before buying or charging a
LiPo.

| Qty | Part | Notes |
|-----|------|-------|
| 1 | 12 V battery, for example a 3S LiPo (11.1 V) | Not yet selected: wait before buying. Must supply about 8 A in short bursts (three motors at the board's rating of 2.5 A each). |
| 1 | Battery lead with an XT60 plug | The lead on the photographed robot splits into two red and black pairs with tinned ends, one in VM and one in VIN |
| 1 | LiPo balance charger | Not yet selected |
| 1 | LiPo alarm (optional) | Sounds when the battery runs low; the robot does not measure its battery voltage. |

## Printed parts and fasteners

The print files are not published yet; they will be in [3d_models/](3d_models/README.md).
The [hardware README](README.md#3d-printing-settings) gives the printing advice.

| Qty | Part | Notes |
|-----|------|-------|
| 1 | Motor plate | Lower hexagon plate with three motor channels on its underside. Print it first and check that a motor fits. |
| 1 | Top plate | Upper hexagon plate |
| 1 | Board frame | Square window and four pegs for the board |
| 1 | Battery holder | |
| 1 | Antenna bracket | |
| 3 | Pillar | |
| 3 | Hub | Joins a wheel to a motor shaft |

Screw and nut sizes are to be confirmed. The counts below come from the photographs and are to be
rechecked against the chassis files.

- Hex-socket button-head screws: 2 per motor, 2 on the frame, 2 on the battery holder, 3 under the
  pillars, 3 on the top plate and 2 on the antenna bracket.
- Hex nuts: 4 under the motor plate and 2 on the antenna bracket.

## Consumables

| Qty | Part | Example source | Notes |
|-----|------|----------------|-------|
| 1 | Masking tape | Any | Temporary marks on the motor wires during assembly |

Tools are listed in [Getting started](../docs/01_Getting_Started.md#tools).
