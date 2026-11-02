# Tools

None of these is needed to build or drive the robot. Run each from the project folder.

| Tool | What it does | Needs |
|------|--------------|-------|
| `check_all.sh` | Builds every board and lesson, runs the host test and the picture check, and prints a table of results | PlatformIO |
| `host_test/run.sh` | Runs the robot program on this computer, with simulated motors and encoders | A C++ compiler |
| `embed_web.py` | Packs `web/` into `firmware/OmniBots_Mini/web_assets.h`; `--check` only compares them | Python 3 |
| `fake_robot.py` | Serves the driving page and answers it like the robot, for page work without hardware | Python 3, uvicorn, Starlette |
| `make_drawings.py` | Draws the diagrams in `docs/images/drawings/` | Python 3 |
| `process_photos.py` | Makes `docs/images/photos/` from the full-size originals; these are not in the repository and must be placed locally in `docs/images/originals/` | Python 3, numpy, OpenCV, Pillow |
| `label_photos.py` | Draws part labels on the processed photos and writes `docs/images/parts/`, `assembly/` and `wiring/` | Python 3, Pillow |
| `show_pictures.py` | Hides a picture on a page if its file is missing and shows it once the file exists; `--check` changes nothing | Python 3 |
