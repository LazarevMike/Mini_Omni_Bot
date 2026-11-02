"""Draws part labels on the processed photos and writes the labelled pictures.

    python3 tools/label_photos.py                 (every picture)
    python3 tools/label_photos.py battery_lead    (the pictures whose file name contains this)

The script reads docs/images/photos/ and writes to docs/images/parts/, assembly/ and wiring/. It
needs only Pillow, and it does not need the full-size originals. The same photos always give the
same files. Only labels are drawn; nothing in a photo is changed.

Every picture has one look: a thin leader line from a white pill to a small dot on the part. The
sizes are set for a page about 800 pixels wide (DISPLAY_WIDTH), so that the text is the same size
on every picture of a page, whatever the pixel width of the file.

All positions in PICTURES are pixels of the processed photo, after any quarter turn, so they can
be read directly from an image viewer. A crop may reach outside a photo with a bare edge: the edge
is then mirrored, to give the labels room. To move a label, change its numbers and look at the
result at full size: a label on the wrong part is the worst mistake a picture can make. Name a
wheel M1, M3 or M4 only when the photo proves it.
"""

import argparse
import io
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont, ImageOps

ROOT = Path(__file__).resolve().parent.parent  # the project folder, one up from tools/
PHOTOS = ROOT / "docs" / "images" / "photos"
IMAGES = ROOT / "docs" / "images"

DISPLAY_WIDTH = 800   # the width, in pixels, at which a page shows a picture
FONT_SIZE = 15        # all sizes below are pixels at DISPLAY_WIDTH
LINE_WIDTH = 1.2
HALO_WIDTH = 3.2      # a white edge keeps a line visible on dark plates and on the white table
DOT_RADIUS = 3.0
PAD_X = 8.5
PAD_Y = 4.5
ROW_GAP = 1.22        # distance between the rows of a pill, as a multiple of FONT_SIZE

MIN_FILE_WIDTH = 1200  # a smaller crop is enlarged to this width, so that the text stays sharp
MAX_BYTES = 400_000
SUPERSAMPLE = 3        # lines and pills are drawn this much larger, then reduced, to smooth them

INK = (31, 37, 43)
WHITE = (255, 255, 255)
PILL_EDGE = (154, 165, 177)

REGULAR_FONTS = [
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "C:/Windows/Fonts/arial.ttf",
    "/System/Library/Fonts/Supplemental/Arial.ttf",
]
BOLD_FONTS = [
    "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
    "C:/Windows/Fonts/arialbd.ttf",
    "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
]


def L(text, pill, *targets):
    """One label: its text (a tuple for two rows), the pill's centre, the points that it names."""
    rows = (text,) if isinstance(text, str) else tuple(text)
    return {"rows": rows, "pill": pill, "targets": targets}


def B(text, pill, x0, x1, y, tick=11):
    """A label with a bracket over a group of parts, from x0 to x1 at height y, instead of a dot."""
    label = L(text, pill)
    label["bracket"] = (x0, x1, y, tick)
    return label


# Each picture: the file it writes, the photo it reads, the labels, and optionally a quarter turn
# ("turn", degrees anticlockwise) and a crop (left, top, right, bottom).
PICTURES = [
    # Getting started and Assembly, You will need.
    dict(out="parts/all_parts.jpg", photo="IMG_6942", labels=[
        L("Motor cables (3)", (880, 215), (640, 300)),
        L("XT60 battery lead", (545, 622), (692, 532)),
        L("Battery holder", (272, 490), (272, 575)),
        L("Board frame", (100, 640), (188, 800)),
        L("Romeo board", (488, 958), (500, 865)),
        L("Hubs (3)", (725, 620), (722, 672)),
        L("Wheels (3)", (1085, 790), (905, 790)),
        L("Antenna on its bracket", (1005, 1028), (760, 1003)),
        L("Top plate", (110, 1490), (250, 1345)),
        L("Motor plate", (1125, 1520), (1015, 1345)),
        L("Motors (3)", (1125, 1130), (905, 1175)),
    ]),

    # Assembly, steps 1 to 11.
    dict(out="assembly/motors_in_channels.jpg", photo="IMG_6943", labels=[
        L("Motor", (1320, 700), (985, 850)),
        L("Shaft", (1330, 790), (1128, 850)),
        L("Channel", (1320, 900), (1085, 908)),
        L("Cable plug", (1130, 560), (872, 810)),
        L("Encoder", (1010, 1110), (886, 878)),
        L("Dupont connector", (1240, 1500), (983, 1500)),
    ]),
    dict(out="assembly/hub_and_wheel.jpg", photo="IMG_6944", labels=[
        L("Roller", (180, 110), (340, 330)),
        L("Wheel body", (250, 850), (530, 690)),
        L("Hub: hexagonal body", (280, 1050), (690, 1090)),
        L("Hub: stem", (420, 1290), (760, 1268)),
    ]),
    dict(out="assembly/hub_in_wheel.jpg", photo="IMG_6945", labels=[
        L("Roller", (300, 400), (590, 500)),
        L("Wheel body", (1300, 280), (1020, 385)),
        L("Hub", (1310, 560), (925, 640)),
        L("Stem and bore", (1320, 800), (990, 685)),
    ]),
    dict(out="assembly/wheel_going_on.jpg", photo="IMG_6946", labels=[
        L("Motor plate, top side", (1320, 650), (1120, 650)),
        L("Motor screws", (840, 790), (811, 903), (877, 898)),
        L("Shaft", (1040, 960), (845, 925)),
        L("Wheel", (480, 1140), (760, 1055)),
        L("Hub", (250, 280), (505, 370)),
    ]),
    dict(out="assembly/frame_on_plate.jpg", photo="IMG_6948", labels=[
        L("Board frame", (300, 290), (640, 535)),
        L("Peg", (250, 610), (515, 555)),
        L("Window", (640, 1000), (640, 800)),
        L("Centre slot", (1020, 610), (785, 700)),
        L("Button-head screws", (1280, 1260), (1002, 1175)),
        L("Nuts", (1200, 1370), (975, 1275), (1040, 1255)),
    ]),
    dict(out="assembly/frame_close_up.jpg", photo="IMG_6949", labels=[
        L("Peg", (140, 400), (318, 545)),
        L("Button-head screw", (190, 760), (265, 590)),
        L("Board frame", (1060, 170), (1000, 345)),
        L("Window", (640, 1000), (640, 620)),
        L("Centre slot", (1370, 640), (980, 640)),
    ]),
    dict(out="assembly/cables_through_slot.jpg", photo="IMG_6950", labels=[
        L("Motor cables", (1210, 200), (1012, 500)),
        L("Centre slot", (560, 800), (885, 880)),
        L("Board frame", (1420, 840), (1262, 900)),
        L("Peg", (620, 560), (820, 695)),
        L("Frame screw", (1410, 580), (1282, 686)),
        L("Frame screw", (560, 1150), (786, 1115)),
    ]),
    dict(out="assembly/board_on_pegs.jpg", photo="IMG_6937", labels=[
        L("Peg in a corner hole", (720, 45), (402, 146)),
        L("Board frame", (130, 340), (310, 400)),
        L("Frame screw", (130, 1130), (300, 1268)),
        L("USB-C port", (880, 1500), (875, 1330)),
        L("Romeo board", (1060, 640), (900, 690)),
    ]),
    dict(out="assembly/encoder_leads.jpg", photo="IMG_6954", labels=[
        L("Dupont connectors", (250, 60), (560, 370)),
        L("Heat-shrink sleeve", (1200, 310), (1100, 190)),
        L("IO17 stays free", (1120, 610), (823, 435)),
        L("Encoder header", (340, 850), (560, 535)),
        L("M1 terminal", (120, 720), (220, 500)),
    ]),
    dict(out="assembly/holder_beside_board.jpg", photo="IMG_6936", labels=[
        L("Battery holder", (1360, 560), (1040, 760)),
        L("XT60 plug", (1240, 330), (1075, 590)),
        L("Holder screw", (960, 430), (876, 575)),
        L("Holder screw", (1330, 1140), (1120, 1062)),
        L("M1, M3 terminals", (1000, 940), (745, 730), (748, 862)),
        L("USB-C port", (330, 1240), (478, 1080)),
    ]),
    dict(out="assembly/pillars_on_plate.jpg", photo="IMG_6958", labels=[
        L("Pillar", (100, 60), (236, 190)),
        L("Pillar", (100, 960), (262, 720)),
        L("Pillar", (1490, 280), (1432, 480)),
    ]),
    dict(out="assembly/antenna_on_plate.jpg", photo="IMG_6959", labels=[
        L("Antenna", (340, 90), (620, 230)),
        L("Antenna bracket", (1300, 420), (1020, 450)),
        L("Antenna lead", (1260, 250), (862, 370)),
        L("Plate hole", (500, 570), (866, 458)),
        L("Top plate", (200, 730), (300, 590)),
        L("U.FL plug on its socket", (1420, 1040), (1182, 898)),
    ]),
    dict(out="assembly/bracket_nuts.jpg", photo="IMG_6960", labels=[
        L("Nut", (480, 250), (768, 378)),
        L("Nut", (1200, 500), (978, 450)),
        L("Antenna", (1270, 240), (1040, 310)),
        L("Antenna bracket", (440, 400), (705, 440)),
        L("Antenna lead", (560, 110), (838, 410)),
    ]),
    dict(out="assembly/finished_top.jpg", photo="IMG_6962", labels=[
        L("Top plate", (1100, 110), (1000, 460)),
        L("Top-plate screw", (260, 210), (505, 335)),
        L("Top-plate screw", (160, 1060), (320, 960)),
        L("Top-plate screw", (1300, 1370), (1082, 1182)),
        L("Antenna bracket", (600, 880), (425, 880)),
    ]),
    dict(out="assembly/finished_underside.jpg", photo="IMG_6963", labels=[
        L("Motor", (900, 1300), (1000, 860)),
        L("Cable plug", (1000, 590), (855, 722)),
        L("Wheel", (1400, 1200), (1400, 900)),
        L("Nut", (200, 420), (335, 527)),
    ]),

    # How it works.
    dict(out="assembly/underside_motors.jpg", photo="IMG_6939", labels=[
        L("Wheel", (130, 480), (210, 660)),
        L("Motor", (420, 480), (470, 690)),
    ]),

    # Wiring.
    dict(out="wiring/romeo_overview_labelled.jpg", photo="IMG_6938", turn=270,
         crop=(-90, 0, 1730, 1425), labels=[
             L("Header with IO0", (330, 120), (405, 243)),
             L("M2 (unused)", (690, 120), (730, 222)),
             L("M4", (945, 120), (945, 218)),
             L("RST", (1090, 120), (1090, 242)),
             L("BOOT", (1250, 120), (1238, 236)),
             L("Antenna socket", (1530, 598), (1266, 598)),
             L("VIN", (1560, 868), (1402, 868)),
             L("JP1", (1560, 962), (1214, 1000)),
             L("VM", (1560, 1056), (1402, 1056)),
             L("Encoder header", (205, 1340), (405, 1212)),
             L("PMODE", (452, 1340), (577, 1031)),
             L("M1EN, M1PH", (660, 1340), (578, 1184), (629, 1184)),
             L("M1", (850, 1340), (735, 1212)),
             L("M3", (985, 1340), (945, 1212)),
             L("SW", (1140, 1340), (1178, 1176)),
             L("PWR", (1290, 1340), (1204, 1073)),
             L("USB-C", (60, 725), (212, 725)),
             L("5 V header", (65, 945), (208, 945)),
         ]),
    dict(out="wiring/jumpers_fitted.jpg", photo="IMG_6938", turn=270,
         crop=(490, 855, 1270, 1310), labels=[
             L("PMODE", (577, 905), (577, 1020)),
             L("JP1", (1213, 905), (1213, 1000)),
             L("M1EN", (545, 1272), (578, 1184)),
             L("M1PH", (660, 1272), (629, 1190)),
         ]),
    dict(out="wiring/motor_terminals_m1_m3.jpg", photo="IMG_6952", labels=[
        L("M3 terminal", (130, 700), (262, 690)),
        L("M1 terminal", (410, 900), (540, 720)),
        L("Red wire on +", (250, 100), (362, 560)),
        L("White wire on \u2212", (160, 290), (275, 555)),
    ]),
    dict(out="wiring/motor_terminal_m4.jpg", photo="IMG_6937", crop=(90, 0, 1000, 1000), labels=[
        L("M4 terminal", (720, 520), (450, 530)),
        L("M2 stays empty", (720, 780), (450, 770)),
        L("Red wire on +", (190, 330), (260, 540)),
        L("White wire on \u2212", (200, 720), (318, 650)),
    ]),
    dict(out="wiring/encoder_layers.jpg", photo="IMG_6953", labels=[
        L("Blue wires on the 3V3 row", (1150, 280), (700, 480)),
        L("Black wires on the GND row", (250, 60), (440, 410)),
        L("IO row, still empty", (1020, 860), (770, 730)),
    ]),
    dict(out="wiring/encoder_header.jpg", photo="IMG_6954", crop=(329, 0, 1202, 754), labels=[
        B(("M1", "IO4, IO5"), (568, 352), 519, 618, 306),
        B(("M3", "IO6, IO7"), (674, 352), 625, 723, 306),
        B(("M4", "IO15, IO16"), (776, 352), 730, 823, 306),
        L("IO17 is free", (1040, 240), (822, 434)),
        L(("Black wires: GND row", "IO4, IO5, IO6 columns"), (470, 58), (464, 131)),
        L(("Blue wires: 3V3 row", "IO4, IO5, IO6 columns"), (830, 58), (671, 158)),
    ]),
    dict(out="wiring/battery_lead.jpg", photo="IMG_6955", labels=[
        L("VM", (660, 560), (810, 395)),
        L("VIN", (1120, 560), (940, 395)),
        L("JP1", (960, 180), (836, 300)),
        L("SW", (540, 290), (722, 290)),
        L("Battery lead, split into two pairs", (1100, 790), (810, 740)),
        L("XT60 plug", (190, 640), (200, 790)),
    ]),
    dict(out="wiring/antenna_clipped_on.jpg", photo="IMG_6941", crop=(119, 556, 833, 992), labels=[
        L(("U.FL plug on the socket", "Press straight down"), (680, 740), (446, 893)),
        L("Antenna lead", (690, 870), (610, 935)),
        L("ESP32-S3 module", (700, 640), (430, 760)),
    ]),
]


def load_font(candidates, size):
    """Returns the first installed font of a list, at a size in pixels."""
    for path in candidates:
        if Path(path).is_file():
            return ImageFont.truetype(path, size)
    raise SystemExit("No sans-serif font found; add one to the font lists in label_photos.py.")


def read_photo(entry):
    """Returns the framed photo of a picture and its crop box in photo pixels."""
    path = PHOTOS / (entry["photo"] + ".jpg")
    if not path.is_file():
        raise SystemExit("%s is missing. The processed photos are part of the repository, so "
                         "restore it from git, or make it again with "
                         "tools/process_photos.py." % path)
    image = Image.open(path).convert("RGB")
    turn = entry.get("turn", 0)
    if turn == 90:
        image = image.transpose(Image.ROTATE_90)
    elif turn == 180:
        image = image.transpose(Image.ROTATE_180)
    elif turn == 270:
        image = image.transpose(Image.ROTATE_270)
    return image, entry.get("crop", (0, 0, image.width, image.height))


def mirrored_crop(image, box):
    """Crops a picture; where the box passes an edge of the photo, that edge is mirrored."""
    left, top, right, bottom = box
    pad_left, pad_top = max(0, -left), max(0, -top)
    pad_right, pad_bottom = max(0, right - image.width), max(0, bottom - image.height)
    wide = Image.new("RGB", (image.width + pad_left + pad_right, image.height), WHITE)
    wide.paste(image, (pad_left, 0))
    if pad_left:
        strip = image.crop((0, 0, pad_left, image.height))
        wide.paste(ImageOps.mirror(strip), (0, 0))
    if pad_right:
        strip = image.crop((image.width - pad_right, 0, image.width, image.height))
        wide.paste(ImageOps.mirror(strip), (image.width + pad_left, 0))
    tall = Image.new("RGB", (wide.width, wide.height + pad_top + pad_bottom), WHITE)
    tall.paste(wide, (0, pad_top))
    if pad_top:
        strip = wide.crop((0, 0, wide.width, pad_top))
        tall.paste(ImageOps.flip(strip), (0, 0))
    if pad_bottom:
        strip = wide.crop((0, wide.height - pad_bottom, wide.width, wide.height))
        tall.paste(ImageOps.flip(strip), (0, wide.height + pad_top))
    return tall.crop((left + pad_left, top + pad_top, right + pad_left, bottom + pad_top))


def edge_point(cx, cy, half_w, half_h, tx, ty):
    """Returns the point where the line from a pill's centre to a target leaves the pill."""
    dx, dy = tx - cx, ty - cy
    if dx == 0 and dy == 0:
        return cx, cy
    scale = min(half_w / abs(dx) if dx else float("inf"), half_h / abs(dy) if dy else float("inf"))
    return cx + dx * scale, cy + dy * scale


def crosses(x0, y0, x1, y1, cx, cy, half_w, half_h):
    """Returns whether the line from (x0, y0) to (x1, y1) passes through a pill."""
    return any(abs(x0 + (x1 - x0) * n / 200 - cx) < half_w
               and abs(y0 + (y1 - y0) * n / 200 - cy) < half_h for n in range(201))


def check_layout(name, pills, box_size):
    """Stops the run if a pill leaves the picture or covers a pill, a target or another line."""
    width, height = box_size
    problems = []
    for n, (rows, cx, cy, half_w, half_h, _, targets) in enumerate(pills):
        if cx - half_w < 0 or cy - half_h < 0 or cx + half_w > width or cy + half_h > height:
            problems.append("pill %r leaves the picture" % (rows,))
        for other_rows, ox, oy, other_w, other_h, _, other_targets in pills[n + 1:]:
            if abs(cx - ox) < half_w + other_w and abs(cy - oy) < half_h + other_h:
                problems.append("pills %r and %r overlap" % (rows, other_rows))
        for other in pills:
            for tx, ty in other[6]:
                if abs(tx - cx) < half_w and abs(ty - cy) < half_h:
                    problems.append("pill %r covers a target of %r" % (rows, other[0]))
        for tx, ty in targets:
            if not (0 <= tx <= width and 0 <= ty <= height):
                problems.append("a target of %r is outside the picture" % (rows,))
            for other in pills:
                if other is not pills[n] and crosses(cx, cy, tx, ty, *other[1:5]):
                    problems.append("a line of %r crosses pill %r" % (rows, other[0]))
    if problems:
        raise SystemExit("%s: %s" % (name, "; ".join(problems)))


def draw_picture(entry):
    """Returns the labelled picture of one entry of PICTURES."""
    image, box = read_photo(entry)
    base = mirrored_crop(image, box)
    file_width = max(base.width, MIN_FILE_WIDTH)
    zoom = file_width / base.width
    if zoom != 1:
        base = base.resize((file_width, round(base.height * zoom)), Image.LANCZOS)

    # s turns a size at DISPLAY_WIDTH into pixels of the supersampled layer.
    s = file_width / DISPLAY_WIDTH * SUPERSAMPLE
    layer = Image.new("RGBA", (base.width * SUPERSAMPLE, base.height * SUPERSAMPLE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(layer)
    regular = load_font(REGULAR_FONTS, round(FONT_SIZE * s))
    bold = load_font(BOLD_FONTS, round(FONT_SIZE * s))

    def place(point):
        return ((point[0] - box[0]) * zoom * SUPERSAMPLE, (point[1] - box[1]) * zoom * SUPERSAMPLE)

    pills = []
    for label in entry["labels"]:
        rows = label["rows"]
        fonts = [bold if len(rows) > 1 and n == 0 else regular for n in range(len(rows))]
        text_width = max(draw.textlength(row, font=font) for row, font in zip(rows, fonts))
        half_w = text_width / 2 + PAD_X * s
        half_h = (FONT_SIZE * s * (1 + ROW_GAP * (len(rows) - 1))) / 2 + PAD_Y * s
        cx, cy = place(label["pill"])
        pills.append((rows, cx, cy, half_w, half_h, fonts, [place(t) for t in label["targets"]]))
    check_layout(entry["out"], pills, layer.size)

    lines = []
    for rows, cx, cy, half_w, half_h, _, targets in pills:
        for target in targets:
            lines.append((edge_point(cx, cy, half_w, half_h, *target), target))
    brackets = []
    for label in entry["labels"]:
        if "bracket" in label:
            x0, x1, y, tick = label["bracket"]
            brackets.append([place((x0, y + tick)), place((x0, y)), place((x1, y)),
                             place((x1, y + tick))])

    for start, end in lines:
        draw.line([start, end], fill=WHITE, width=round(HALO_WIDTH * s))
    for points in brackets:
        draw.line(points, fill=WHITE, width=round(HALO_WIDTH * s), joint="curve")
    for start, end in lines:
        draw.line([start, end], fill=INK, width=round(LINE_WIDTH * s))
    for points in brackets:
        draw.line(points, fill=INK, width=round(LINE_WIDTH * s), joint="curve")
    for _, (tx, ty) in lines:
        r = DOT_RADIUS * s
        draw.ellipse([tx - r, ty - r, tx + r, ty + r], fill=INK, outline=WHITE,
                     width=round(1.1 * s))

    for rows, cx, cy, half_w, half_h, fonts, _ in pills:
        draw.rounded_rectangle([cx - half_w, cy - half_h, cx + half_w, cy + half_h],
                               radius=min(half_h, 11 * s), fill=WHITE, outline=PILL_EDGE,
                               width=max(1, round(0.8 * s)))
        step = FONT_SIZE * s * ROW_GAP
        first = cy - step * (len(rows) - 1) / 2
        for n, (row, font) in enumerate(zip(rows, fonts)):
            draw.text((cx, first + n * step), row, font=font, fill=INK, anchor="mm")

    overlay = layer.resize(base.size, Image.LANCZOS)
    base.paste(overlay, (0, 0), overlay)
    return base


def save_jpeg(image, path):
    """Saves a picture as a plain JPEG with no metadata, at most MAX_BYTES if the quality allows."""
    path.parent.mkdir(parents=True, exist_ok=True)
    quality = 86
    while True:
        buffer = io.BytesIO()
        image.save(buffer, "JPEG", quality=quality, optimize=True, progressive=True,
                   subsampling="4:2:0")
        if buffer.tell() <= MAX_BYTES or quality <= 76:
            break
        quality -= 1
    path.write_bytes(buffer.getvalue())
    return buffer.tell(), quality


def main():
    parser = argparse.ArgumentParser(description="Label the photos in docs/images/photos/.")
    parser.add_argument("names", nargs="*", help="part of a file name; default: every picture")
    args = parser.parse_args()

    chosen = [entry for entry in PICTURES
              if not args.names or any(name in entry["out"] for name in args.names)]
    if not chosen:
        raise SystemExit("No picture matches %s. The pictures are:\n  %s"
                         % (", ".join(args.names),
                            "\n  ".join(entry["out"] for entry in PICTURES)))
    for entry in chosen:
        picture = draw_picture(entry)
        size, quality = save_jpeg(picture, IMAGES / entry["out"])
        print("%-40s %4dx%-4d %3d KB  q%d  %d labels, from %s"
              % (entry["out"], picture.width, picture.height, size // 1024, quality,
                 len(entry["labels"]), entry["photo"]))
    print("%d picture(s) written." % len(chosen))


if __name__ == "__main__":
    main()
