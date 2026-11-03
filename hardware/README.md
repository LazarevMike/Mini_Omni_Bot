# Hardware

This folder is for the design files of Mini Omni Bot's chassis. The 3D models and drawings are
not in the repository yet and will be added once the chassis is final. Until then, the parts list
and [Assembly](../docs/02_Assembly.md) describe the printed parts from the photographed robot.

| Folder or file | Contents | Use |
|----------------|----------|-----|
| [BOM.md](BOM.md) | Every part for one robot, with example suppliers | Buying the parts |
| [3d_models/](3d_models/README.md) | To come: a STEP and an STL file for each printed part | Printing (STL) or editing in CAD (STEP) |
| [drawings/](drawings/README.md) | To come: PDF drawings with dimensions and hole positions | Making or checking a part |
| [LICENSE](LICENSE) | CERN Open Hardware Licence v2, Permissive | Terms of use |

The [Omni Wheels by Omnia](https://www.rotacaster.com.au/shop/50mm-omni-wheels/index), available
from Omnia Wheel, and the electronics are bought, not made;
DFRobot publishes the Romeo board's CAD drawing and 3D model on its
[wiki](https://wiki.dfrobot.com/dfr0994/). [Assembly](../docs/02_Assembly.md) describes the
build.

## File names (planned)

Each made part has a number and a short name, shared by its model, print file, drawing and
parts-list line: `P` marks a part and `A` an assembly, and a new version keeps the number and
increments `v`, as in `P01_motor_plate_v1.step`, `P01_motor_plate_v1.stl`,
`P01_motor_plate_v1.pdf` and `A00_robot_v1.step`.

The made parts are the motor plate, the top plate, the board frame, the battery holder, the
antenna bracket, the pillars and the hubs.

## 3D printing settings

Print settings will be published with the 3D models. Print the motor plate first and check that
a motor fits its channel, remove all support material from screw holes and slots, and tighten
screws into plastic only until snug.

## Changing a part

Edit the STEP file in any CAD program (FreeCAD is free); STL files are meshes and are difficult
to edit. Keep the part number, increase the version, and share improvements as a pull request
on the
[project's GitHub page](https://github.com/LazarevMike/Mini_Omni_Bot).

## Licence

Copyright 2026 Omnia Wheel. Everything in this folder is licensed under the
[CERN Open Hardware Licence v2, Permissive](LICENSE): it may be used, modified, made, shared and
sold, provided the licence notice is kept and the source is acknowledged.
