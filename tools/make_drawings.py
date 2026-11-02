"""Draws the concept pictures for the docs, as SVG files in docs/images/.

Run it from anywhere:

    python3 tools/make_drawings.py

It writes these files, and overwrites them if they exist. In docs/images/drawings/:

    omni_wheel_directions.svg    one omni wheel: the way it rolls and slides, face on and from above
    wheel_angles_top_view.svg    the robot from above: where M1, M3 and M4 sit, and their angles
    kiwi_layout_top.svg          the same, plus the way each wheel pushes with a plus number
    move_shadow.svg              the shadow idea: how much of a move one wheel takes
    kiwi_forward.svg             going forward: each wheel's push, and the parts that cancel
    kiwi_slide_left.svg          sliding left: the same
    drive_commands_top_view.svg  which way w, s, a, d, go 45, q and e move the robot
    h_bridge.svg                 the H-bridge's four states: forward, reverse, brake, coast
    pwm_duty.svg                 three PWM signals: 25%, 50% and 75% duty cycle
    wiring_diagram.svg           the Romeo, three motors and the battery, wire by wire
    encoder_ab.svg               the encoder's A and B signals, each change counted
    push_direction.svg           how to read a wheel's push direction, and flipMotor
    drv8876_one_motor.svg        one Pololu DRV8876 board wired to a motor and a controller

The wheel angles below are the ones in WHEEL_SETUP (firmware/OmniBots_Mini/config.h), and the
wheel powers come from the same formula as drive.cpp. For a robot with other angles, change
WHEEL_ANGLES and run this again. The colours and sizes are at the top too.

It uses only Python's standard library. After adding a new picture, run
python3 tools/show_pictures.py so the docs show it.
"""

import math
from pathlib import Path

IMAGES = Path(__file__).resolve().parent.parent / "docs" / "images"

# Where each wheel sits, in degrees, anticlockwise from the front (seen from above).
WHEEL_ANGLES = {"M1": 60, "M3": 180, "M4": 300}

# One look for every drawing. Blue is a wheel's push, orange is where the robot goes,
# grey dashed is a part that cancels out.
FONT = "Arial, Helvetica, sans-serif"
INK = "#1f2933"          # text
MUTED = "#5f6b7a"        # quieter text, angle marks
CARD = "#ffffff"         # the white card behind everything (legible in dark mode too)
CARD_EDGE = "#d5dbe3"
BODY = "#eef1f5"         # the robot's base
BODY_EDGE = "#9aa5b1"
WHEEL = "#3a3f47"        # wheels
WHEEL_LINE = "#8c96a3"   # the roller stripes on a wheel
PUSH = "#1c6fd6"         # a wheel's push
MOTION = "#e8590c"       # the robot's move
CANCEL = "#97a1ac"       # parts that cancel out

WIDTH = 640              # every drawing is this wide; it stays legible at 400 px on a phone


# ------------------------------------------------------------------------------------
#  The mathematics, the same as drive.cpp
# ------------------------------------------------------------------------------------

def push_direction(angle):
    """Which way a wheel at this angle pushes the robot on positive power, as (forward,
    left). Always at right angles to the line from the centre, anticlockwise round it."""
    a = math.radians(angle)
    return -math.sin(a), math.cos(a)


def wheel_powers(forward, left):
    """Each wheel's power for a straight move, scaled like drive.cpp: the busiest wheel
    gets exactly the speed asked for."""
    raw = {}
    for name, angle in WHEEL_ANGLES.items():
        f, l = push_direction(angle)
        raw[name] = f * forward + l * left
    busiest = max(abs(p) for p in raw.values())
    speed = math.hypot(forward, left)
    return {name: p * speed / busiest for name, p in raw.items()}


# ------------------------------------------------------------------------------------
#  Small drawing helpers. Screen y goes down, so the robot's front is "up".
# ------------------------------------------------------------------------------------

def esc(text):
    return text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def num(value):
    """A power for a label: +1, -0.5, 0, with a real minus sign."""
    if abs(value) < 0.005:
        return "0"
    text = f"{abs(value):.2f}".rstrip("0").rstrip(".")
    return ("+" if value > 0 else "−") + text


def on_screen(cx, cy, forward, left):
    """A point (forward, left) from the robot's centre, in screen pixels."""
    return cx - left, cy - forward


def at_angle(cx, cy, radius, angle):
    """The point at this angle (anticlockwise from the front) and distance from the centre."""
    a = math.radians(angle)
    return cx - radius * math.sin(a), cy - radius * math.cos(a)


def text(x, y, words, size=24, colour=INK, weight="normal", anchor="middle", halo=False):
    extra = ""
    if halo:  # a white edge round the letters, so they read on top of a line
        extra = f' stroke="{CARD}" stroke-width="6" stroke-linejoin="round" paint-order="stroke"'
    return (f'<text x="{x:.1f}" y="{y:.1f}" font-size="{size}" fill="{colour}" '
            f'font-weight="{weight}" text-anchor="{anchor}"{extra}>{esc(words)}</text>')


def head(x, y, ux, uy, colour, size):
    """An arrowhead with its tip at (x, y), pointing along (ux, uy)."""
    bx, by = x - ux * size, y - uy * size
    px, py = -uy * size * 0.62, ux * size * 0.62
    return (f'<polygon points="{x:.1f},{y:.1f} {bx + px:.1f},{by + py:.1f} '
            f'{bx - px:.1f},{by - py:.1f}" fill="{colour}"/>')


def arrow(x1, y1, x2, y2, colour, width=6, dashed=False, both=False, halo=False):
    """A straight arrow. The line stops where the head starts, so the tip stays sharp."""
    size = width * 3.2
    length = math.hypot(x2 - x1, y2 - y1)
    ux, uy = (x2 - x1) / length, (y2 - y1) / length
    sx, sy = (x1 + ux * size, y1 + uy * size) if both else (x1, y1)
    ex, ey = x2 - ux * size, y2 - uy * size
    dash = f' stroke-dasharray="{width * 2:.0f} {width * 1.5:.0f}"' if dashed else ""
    parts = []
    if halo:
        parts.append(f'<line x1="{sx:.1f}" y1="{sy:.1f}" x2="{ex:.1f}" y2="{ey:.1f}" '
                     f'stroke="{CARD}" stroke-width="{width + 8}" stroke-linecap="round"/>')
    parts.append(f'<line x1="{sx:.1f}" y1="{sy:.1f}" x2="{ex:.1f}" y2="{ey:.1f}" '
                 f'stroke="{colour}" stroke-width="{width}"{dash}/>')
    parts.append(head(x2, y2, ux, uy, colour, size))
    if both:
        parts.append(head(x1, y1, -ux, -uy, colour, size))
    return "\n".join(parts)


def curved_arrow(cx, cy, radius, start, end, colour, width=6):
    """An arrow along a circle round (cx, cy), from angle start to angle end. Angles count
    anticlockwise from the front, so end > start goes anticlockwise."""
    size = width * 3.2
    turn = 1 if end > start else -1
    stop = end - turn * math.degrees(size / radius)  # where the line ends and the head starts
    x1, y1 = at_angle(cx, cy, radius, start)
    x2, y2 = at_angle(cx, cy, radius, stop)
    tx, ty = at_angle(cx, cy, radius, end)
    big = 1 if abs(stop - start) > 180 else 0
    sweep = 0 if turn > 0 else 1  # on screen, anticlockwise is SVG's sweep 0
    length = math.hypot(tx - x2, ty - y2)
    return (f'<path d="M {x1:.1f} {y1:.1f} A {radius} {radius} 0 {big} {sweep} {x2:.1f} '
            f'{y2:.1f}" fill="none" stroke="{colour}" stroke-width="{width}"/>\n'
            + head(tx, ty, (tx - x2) / length, (ty - y2) / length, colour, size))


def cross(x, y, size=11):
    """The small x on a part that cancels out."""
    return (f'<path d="M {x - size} {y - size} L {x + size} {y + size} M {x - size} '
            f'{y + size} L {x + size} {y - size}" stroke="{CANCEL}" stroke-width="4" '
            f'stroke-linecap="round"/>')


def wheel(x, y, along, length=64, width=24):
    """A wheel seen from above: a dark block, long side along the way it rolls, with
    stripes for the rollers. along = its rolling direction on screen, in degrees."""
    stripes = "".join(
        f'<line x1="{-length / 2 + k * length / 5:.1f}" y1="{-width / 2 + 3}" '
        f'x2="{-length / 2 + k * length / 5:.1f}" y2="{width / 2 - 3}" '
        f'stroke="{WHEEL_LINE}" stroke-width="2"/>' for k in range(1, 5))
    return (f'<g transform="translate({x:.1f} {y:.1f}) rotate({along:.1f})">'
            f'<rect x="{-length / 2}" y="{-width / 2}" width="{length}" height="{width}" '
            f'rx="6" fill="{WHEEL}"/>{stripes}</g>')


def svg(name, height, title, body, width=WIDTH, folder="drawings"):
    """Wraps the parts in a white card and writes the file."""
    page = (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" '
            f'width="{width}" height="{height}" font-family="{FONT}" role="img">\n'
            f'<title>{esc(title)}</title>\n'
            f'<!-- Made by tools/make_drawings.py: change it there and run it again. -->\n'
            f'<rect x="2" y="2" width="{width - 4}" height="{height - 4}" rx="24" '
            f'fill="{CARD}" stroke="{CARD_EDGE}" stroke-width="2"/>\n'
            + "\n".join(body) + "\n</svg>\n")
    (IMAGES / folder).mkdir(parents=True, exist_ok=True)
    (IMAGES / folder / name).write_text(page, encoding="utf-8")
    print(f"wrote docs/images/{folder}/{name}")


def line(x1, y1, x2, y2, colour=INK, width=3, dash=""):
    extra = f' stroke-dasharray="{dash}"' if dash else ""
    return (f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" stroke="{colour}" '
            f'stroke-width="{width}" stroke-linecap="round"{extra}/>')


def path(points, colour, width=4, dash=""):
    """A wire through these points, with rounded joints."""
    extra = f' stroke-dasharray="{dash}"' if dash else ""
    joined = " ".join(f"{x:.1f},{y:.1f}" for x, y in points)
    return (f'<polyline points="{joined}" fill="none" stroke="{colour}" stroke-width="{width}" '
            f'stroke-linecap="round" stroke-linejoin="round"{extra}/>')


def box(x, y, w, h, fill=BODY, edge=BODY_EDGE, radius=8, width=3):
    return (f'<rect x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{h:.1f}" rx="{radius}" '
            f'fill="{fill}" stroke="{edge}" stroke-width="{width}"/>')


def dot(x, y, r=7, fill=INK, edge=None):
    extra = f' stroke="{edge}" stroke-width="3"' if edge else ""
    return f'<circle cx="{x:.1f}" cy="{y:.1f}" r="{r}" fill="{fill}"{extra}/>'


# ------------------------------------------------------------------------------------
#  The robot from above, front up
# ------------------------------------------------------------------------------------

def robot(cx, cy, radius, front_label=True, label_drop=52):
    """The base, the three wheels on its rim, and a front mark. Returns the parts and each
    wheel's centre."""
    parts = [f'<circle cx="{cx}" cy="{cy}" r="{radius - 8}" fill="{BODY}" '
             f'stroke="{BODY_EDGE}" stroke-width="3"/>']
    # The front mark: a small triangle on the rim, like the tape on the real robot.
    tip_y = cy - radius + 8
    parts.append(f'<polygon points="{cx},{tip_y + 2} {cx - 14},{tip_y + 24} '
                 f'{cx + 14},{tip_y + 24}" fill="{INK}"/>')
    if front_label:
        parts.append(text(cx, tip_y + label_drop, "front", 22, INK, "bold"))
    centres = {}
    for name, angle in WHEEL_ANGLES.items():
        x, y = at_angle(cx, cy, radius, angle)
        f, l = push_direction(angle)
        dx, dy = -l, -f  # the rolling direction, on screen
        parts.append(wheel(x, y, math.degrees(math.atan2(dy, dx))))
        centres[name] = (x, y)
    return parts, centres


def push_start(centre, angle, sign, gap=36):
    """Where a wheel's push arrow starts: just off the end of the wheel it points from."""
    f, l = push_direction(angle)
    dx, dy = -l * sign, -f * sign
    return centre[0] + dx * gap, centre[1] + dy * gap, dx, dy


# ------------------------------------------------------------------------------------
#  The drawings
# ------------------------------------------------------------------------------------

def omni_wheel_directions():
    """One omni wheel, face on, its axle going back into the page at a slant. Beside it,
    the same wheel from above, the way the other drawings show wheels."""
    cx, cy, r = 200, 205, 112
    deep_x, deep_y = math.cos(math.radians(35)), -math.sin(math.radians(35))  # the axle
    parts = []
    # The floor, and the back of the wheel, partly visible so that the wheel looks solid.
    parts.append(f'<line x1="40" y1="{cy + r + 16}" x2="400" y2="{cy + r + 16}" '
                 f'stroke="{CARD_EDGE}" stroke-width="4" stroke-linecap="round"/>')
    parts.append(f'<circle cx="{cx + deep_x * 26:.1f}" cy="{cy + deep_y * 26:.1f}" '
                 f'r="{r + 10}" fill="#c4cbd4"/>')
    # The rollers round the rim. Each one's axle runs along the rim, so it turns to let
    # the wheel slide along its own axle.
    for k in range(10):
        a = k * 36 + 18
        x = cx + r * math.cos(math.radians(a))
        y = cy + r * math.sin(math.radians(a))
        parts.append(f'<g transform="translate({x:.1f} {y:.1f}) rotate({a + 90})">'
                     f'<rect x="-27" y="-14" width="54" height="28" rx="14" fill="{WHEEL}"/>'
                     f'<line x1="-18" y1="0" x2="18" y2="0" stroke="{WHEEL_LINE}" '
                     f'stroke-width="3" stroke-linecap="round"/></g>')
    parts.append(f'<circle cx="{cx}" cy="{cy}" r="{r - 16}" fill="#e4e8ed" '
                 f'stroke="{WHEEL}" stroke-width="4"/>')
    for k in range(5):
        x, y = at_angle(cx, cy, 56, k * 72)
        parts.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="15" fill="{CARD}" '
                     f'stroke="{WHEEL_LINE}" stroke-width="3"/>')
    parts.append(f'<circle cx="{cx}" cy="{cy}" r="24" fill="{WHEEL}"/>')
    # Sliding: along the axle, through the hub, kept clear of the floor.
    reach = 170
    parts.append(arrow(cx - deep_x * reach, cy - deep_y * reach, cx + deep_x * reach,
                       cy + deep_y * reach, MUTED, 7, both=True, halo=True))
    parts.append(text(330, 50, "slides this way,", 24, INK, "bold"))
    parts.append(text(330, 78, "on its rollers", 24, INK, "bold"))
    # Rolling: along the floor.
    parts.append(arrow(cx - 160, cy + r + 58, cx + 160, cy + r + 58, PUSH, 8, both=True))
    parts.append(text(cx, cy + r + 104, "rolls and pushes this way", 24, PUSH, "bold"))
    # The same wheel from above: rolls along it, slides across it.
    ix = 528
    parts.append(f'<rect x="440" y="120" width="176" height="300" rx="16" fill="#f7f9fb" '
                 f'stroke="{CARD_EDGE}" stroke-width="2"/>')
    parts.append(text(ix, 156, "from above", 22, MUTED, "bold"))
    parts.append(wheel(ix, 262, 0, 96, 32))
    parts.append(arrow(ix, 192, ix, 332, MUTED, 6, both=True, halo=True))
    parts.append(text(ix + 12, 208, "slides", 22, INK, "bold", anchor="start"))
    parts.append(arrow(ix - 66, 364, ix + 66, 364, PUSH, 7, both=True))
    parts.append(text(ix, 400, "rolls", 22, PUSH, "bold"))
    svg("omni_wheel_directions.svg", cy + r + 130,
        "One omni wheel: it rolls and pushes one way, and slides along its axle", parts)


def angle_marks(cx, cy, radius):
    """0° front, 90° left, 180° back, 270° right round the robot, and an arrow for the
    way angles count."""
    parts = [f'<circle cx="{cx}" cy="{cy}" r="{radius}" fill="none" stroke="{CARD_EDGE}" '
             f'stroke-width="2" stroke-dasharray="6 6"/>']
    for angle, word in ((0, "front"), (90, "left"), (180, "back"), (270, "right")):
        x1, y1 = at_angle(cx, cy, radius - 9, angle)
        x2, y2 = at_angle(cx, cy, radius + 9, angle)
        parts.append(f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" '
                     f'stroke="{MUTED}" stroke-width="3"/>')
        x, y = at_angle(cx, cy, radius + 44, angle)
        if angle == 0:
            y -= 4
        parts.append(text(x, y - 4, f"{angle}°", 24, INK, "bold"))
        parts.append(text(x, y + 22, word, 22, MUTED))
    parts.append(curved_arrow(cx, cy, radius, 12, 40, MUTED, 4))
    x, y = at_angle(cx, cy, radius + 26, 26)
    parts.append(text(x - 6, y - 4, "anticlockwise", 22, MUTED, anchor="end"))
    return parts


def wheel_labels(cx, cy, radius):
    """M1 60°, M3 180°, M4 300°, just inside each wheel."""
    parts = []
    for name, angle in WHEEL_ANGLES.items():
        x, y = at_angle(cx, cy, radius - 58, angle)
        parts.append(text(x, y + 2, name, 24, INK, "bold"))
        parts.append(text(x, y + 26, f"{angle}°", 22, MUTED))
    return parts


def wheel_angles_top_view():
    cx, cy, radius = 320, 330, 150
    parts, _ = robot(cx, cy, radius, front_label=False)
    parts += wheel_labels(cx, cy, radius)
    parts += angle_marks(cx, cy, 212)
    svg("wheel_angles_top_view.svg", 660,
        "The robot from above, front up: M1 at 60 degrees, M3 at 180, M4 at 300", parts)


def kiwi_layout_top():
    cx, cy, radius = 320, 330, 150
    parts, centres = robot(cx, cy, radius, front_label=False)
    parts += wheel_labels(cx, cy, radius)
    parts += angle_marks(cx, cy, 212)
    for name, angle in WHEEL_ANGLES.items():
        x, y, dx, dy = push_start(centres[name], angle, +1)
        parts.append(arrow(x, y, x + dx * 84, y + dy * 84, PUSH, 7))
    parts.append(arrow(40, 666, 96, 666, PUSH, 7))
    parts.append(text(110, 674, "which way each wheel pushes with a plus number", 22, INK,
                      anchor="start"))
    svg("kiwi_layout_top.svg", 706,
        "Top view: each wheel pushes the robot anticlockwise round its centre", parts)


def move_shadow():
    """Three panels: a move along a wheel, at 60° to it, and at right angles."""
    parts = []
    line_y, reach = 210, 110
    cases = ((0, "all of it", "the way it rolls"),
             (60, "half of it", "at 60°"),
             (90, "none of it", "at right angles"))
    for k, (tilt, big, small) in enumerate(cases):
        x0 = 20 + k * 205
        ox = x0 + 72  # where the move starts
        parts.append(f'<line x1="{x0 + 8}" y1="{line_y}" x2="{x0 + 196}" y2="{line_y}" '
                     f'stroke="{CANCEL}" stroke-width="3"/>')
        parts.append(wheel(x0 + 32, line_y, 0, 48, 20))
        tx = ox + reach * math.cos(math.radians(tilt))
        ty = line_y - reach * math.sin(math.radians(tilt))
        if tilt == 90:
            parts.append(f'<circle cx="{ox}" cy="{line_y}" r="8" fill="{PUSH}"/>')
        else:
            parts.append(f'<line x1="{ox}" y1="{line_y}" x2="{tx:.1f}" y2="{line_y}" '
                         f'stroke="{PUSH}" stroke-width="14" stroke-linecap="round" '
                         f'opacity="0.85"/>')
        if 0 < tilt < 90:
            parts.append(f'<line x1="{tx:.1f}" y1="{ty:.1f}" x2="{tx:.1f}" y2="{line_y}" '
                         f'stroke="{CANCEL}" stroke-width="3" stroke-dasharray="7 6"/>')
        parts.append(arrow(ox, line_y, tx, ty, MOTION, 6))
        parts.append(text(x0 + 102, line_y + 50, big, 24, INK, "bold"))
        parts.append(text(x0 + 102, line_y + 78, small, 22, MUTED))
    parts.append(arrow(70, 356, 120, 356, MOTION, 6))
    parts.append(text(132, 364, "the move you want", 22, INK, anchor="start"))
    parts.append(f'<line x1="360" y1="356" x2="400" y2="356" stroke="{PUSH}" '
                 f'stroke-width="14" stroke-linecap="round" opacity="0.85"/>')
    parts.append(text(418, 364, "the wheel's share", 22, INK, anchor="start"))
    svg("move_shadow.svg", 400,
        "The shadow idea: a wheel takes the part of the move that lies along it", parts)


def move_picture(name, title, forward, left, kept_word, scale):
    """Top view of one straight move: each wheel's push (blue, length = power), split
    into the part that helps (orange dashes) and the part that cancels (grey, crossed
    out), and the robot's move in the middle."""
    cx, cy, radius = 320, 350, 150
    powers = wheel_powers(forward, left)
    parts, centres = robot(cx, cy, radius, label_drop=60)
    move_x, move_y = -left, -forward  # the move, on screen (a unit vector here)
    for wheel_name, angle in WHEEL_ANGLES.items():
        power = powers[wheel_name]
        x, y = centres[wheel_name]
        lx, ly = at_angle(cx, cy, radius + 62, angle)
        parts.append(text(lx, ly + 8, f"{wheel_name}  {num(power)}", 26, INK, "bold"))
        if abs(power) < 0.005:
            parts.append(text(lx, ly + 34, "slides along", 22, MUTED))
            continue
        x, y, dx, dy = push_start((x, y), angle, 1 if power > 0 else -1)
        px, py = dx * abs(power) * scale, dy * abs(power) * scale  # the push
        along = px * move_x + py * move_y  # the part of the push along the move
        kx, ky = move_x * along, move_y * along
        if abs(along) < abs(power) * scale - 1:  # some of it goes sideways and cancels
            cx2, cy2 = x + px - kx, y + py - ky
            parts.append(arrow(x, y, cx2, cy2, CANCEL, 4, dashed=True))
            parts.append(cross((x + cx2) / 2, (y + cy2) / 2))
            parts.append(arrow(x, y, x + kx, y + ky, MOTION, 4, dashed=True))
        parts.append(arrow(x, y, x + px, y + py, PUSH, 7))
    # The robot's move, big, from the centre, with its name underneath.
    if move_y:  # going forward or back: the word under the arrow's tail
        parts.append(arrow(cx, cy - move_y * 60, cx, cy + move_y * 55, MOTION, 11))
        parts.append(text(cx, cy + 95, kept_word, 24, MOTION, "bold"))
    else:       # sliding: the word under the arrow's middle
        parts.append(arrow(cx - move_x * 45, cy, cx + move_x * 70, cy, MOTION, 11))
        parts.append(text(cx + move_x * 12, cy + 46, kept_word, 24, MOTION, "bold"))
    # The key.
    height = 2 * cy - 40
    key_y = height - 30
    parts.append(arrow(40, key_y, 88, key_y, PUSH, 7))
    parts.append(text(98, key_y + 8, "wheel push", 22, INK, anchor="start"))
    parts.append(arrow(232, key_y, 280, key_y, MOTION, 4, dashed=True))
    parts.append(text(290, key_y + 8, "helps", 22, INK, anchor="start"))
    parts.append(arrow(370, key_y, 418, key_y, CANCEL, 4, dashed=True))
    parts.append(cross(394, key_y, 8))
    parts.append(text(428, key_y + 8, "cancels out", 22, INK, anchor="start"))
    svg(name, height, title, parts)


def kiwi_forward():
    move_picture("kiwi_forward.svg", "Going forward: M1 at -1, M3 at 0, M4 at +1",
                 1, 0, "forward", scale=175)


def kiwi_slide_left():
    move_picture("kiwi_slide_left.svg", "Sliding left: M1 at +0.5, M3 at -1, M4 at +0.5",
                 0, 1, "left", scale=235)


def drive_commands_top_view():
    cx, cy, radius = 320, 330, 100
    parts, _ = robot(cx, cy, radius)
    for name, angle in WHEEL_ANGLES.items():
        x, y = at_angle(cx, cy, radius - 46, angle)
        parts.append(text(x, y + 8, name, 22, INK, "bold"))
    moves = ((0, "w", "forward"), (180, "s", "back"), (90, "a", "slide left"),
             (270, "d", "slide right"), (45, "go 45", ""))
    for angle, key, words in moves:
        x1, y1 = at_angle(cx, cy, radius + 34, angle)
        x2, y2 = at_angle(cx, cy, radius + 120, angle)
        parts.append(arrow(x1, y1, x2, y2, MOTION, 8))
        lx, ly = at_angle(cx, cy, radius + 168, angle)
        if words:
            parts.append(text(lx, ly - 2, key, 30, INK, "bold"))
            parts.append(text(lx, ly + 24, words, 22, MUTED))
        else:
            parts.append(text(lx, ly + 10, key, 30, INK, "bold"))
    # Turning gets its own small robot, with an arrow right round it, so it reads as
    # turning on the spot and not as a move. Each arrow ends at the front: q swings the
    # front left (anticlockwise), e swings it right (clockwise).
    for x, key, words, start, end in ((115, "q", "turn left", -200, 40),
                                      (525, "e", "turn right", 200, -40)):
        y = 505
        parts.append(f'<circle cx="{x}" cy="{y}" r="30" fill="{BODY}" stroke="{BODY_EDGE}" '
                     f'stroke-width="3"/>')
        parts.append(f'<polygon points="{x},{y - 26} {x - 9},{y - 12} {x + 9},{y - 12}" '
                     f'fill="{INK}"/>')
        parts.append(curved_arrow(x, y, 52, start, end, MOTION, 6))
        parts.append(text(x, 596, key, 30, INK, "bold"))
        parts.append(text(x, 622, words, 22, MUTED))
    svg("drive_commands_top_view.svg", 660,
        "Top view: which way w, s, a, d, go 45, q and e move the robot", parts)


# ------------------------------------------------------------------------------------
#  Motor drivers and encoders
# ------------------------------------------------------------------------------------

def h_bridge():
    """The four states of an H-bridge, in a 2 x 2 grid. S1 and S2 are the top switches, S3 and
    S4 the bottom ones. EN and PH follow the DRV8876's PH/EN table: EN 1, PH 1 drives OUT1 high
    and OUT2 low; EN 0 turns both low-side switches on."""
    panels = (
        ("Forward", "EN 1, PH 1", (True, False, False, True)),
        ("Reverse", "EN 1, PH 0", (False, True, True, False)),
        ("Brake", "EN 0: both motor terminals joined", (False, False, True, True)),
        ("Coast", "Driver asleep: all switches open", (False, False, False, False)),
    )
    parts = []
    for k, (name, note, closed) in enumerate(panels):
        ox, oy = 20 + (k % 2) * 305, 20 + (k // 2) * 322
        left, right, mid = ox + 50, ox + 250, ox + 150
        top, bottom, centre = oy + 104, oy + 272, oy + 188
        parts.append(text(mid, oy + 30, name, 26, INK, "bold"))
        parts.append(text(mid, oy + 56, note, 18, MUTED))
        # The rails.
        parts.append(line(left, top, right, top, INK, 4))
        parts.append(line(left, bottom, right, bottom, INK, 4))
        parts.append(text(mid, top - 12, "VM (+)", 18, INK))
        parts.append(text(mid, bottom + 26, "GND", 18, INK))
        # The switches, as pairs of contacts on each leg.
        legs = ((left, top, centre, closed[0]), (right, top, centre, closed[1]),
                (left, centre, bottom, closed[2]), (right, centre, bottom, closed[3]))
        for x, y1, y2, shut in legs:
            a, b = y1 + 24, y2 - 24
            parts.append(line(x, y1, x, a, INK, 3))
            parts.append(line(x, b, x, y2, INK, 3))
            if shut:
                parts.append(line(x, a, x, b, INK, 5))
            else:  # an open switch: the blade swings away from the far contact
                swing = 22 if x == left else -22
                parts.append(line(x, a, x + swing, b - 2, MUTED, 4))
            parts.append(dot(x, a, 5))
            parts.append(dot(x, b, 5))
        # The motor across the middle.
        parts.append(line(left, centre, mid - 30, centre, INK, 3))
        parts.append(line(mid + 30, centre, right, centre, INK, 3))
        # Current path.
        current = []
        if closed[0] and closed[3]:  # left to right through the motor
            current = [[(mid, top), (left, top), (left, centre), (mid - 30, centre)],
                       [(mid + 30, centre), (right, centre), (right, bottom), (mid, bottom)]]
            parts.append(arrow(left + 8, centre - 13, mid - 36, centre - 13, MOTION, 4))
        elif closed[1] and closed[2]:  # right to left
            current = [[(mid, top), (right, top), (right, centre), (mid + 30, centre)],
                       [(mid - 30, centre), (left, centre), (left, bottom), (mid, bottom)]]
            parts.append(arrow(right - 8, centre - 13, mid + 36, centre - 13, MOTION, 4))
        elif closed[2] and closed[3]:  # the motor's terminals joined through GND
            current = [[(mid - 30, centre), (left, centre), (left, bottom), (right, bottom),
                        (right, centre), (mid + 30, centre)]]
        for run in current:
            parts.append(path(run, MOTION, 5))
        parts.append(f'<circle cx="{mid}" cy="{centre}" r="30" fill="{BODY}" stroke="{INK}" '
                     f'stroke-width="3"/>')
        parts.append(text(mid, centre + 9, "M", 26, INK, "bold"))
    parts.append(line(190, 668, 240, 668, MOTION, 5))
    parts.append(text(252, 675, "the path of the motor current", 20, INK, anchor="start"))
    svg("h_bridge.svg", 696, "An H-bridge in four states: forward, reverse, brake and coast",
        parts)


def pwm_duty():
    """Three PWM signals with the same period. analogWrite takes 0 to 255."""
    rows = ((25, 64, "about a quarter of full power"), (50, 128, "about half of full power"),
            (75, 191, "about three quarters of full power"))
    x0, period, cycles = 40, 186, 3
    parts = []
    for k, (duty, value, words) in enumerate(rows):
        base = 124 + k * 158
        high = base - 62
        parts.append(text(x0, high - 18, f"{duty}% duty cycle", 22, INK, "bold", "start"))
        parts.append(text(x0 + 182, high - 18, f"analogWrite({value}): {words}", 17, MUTED,
                          anchor="start"))
        points = [(x0, base)]
        for c in range(cycles):
            start = x0 + c * period
            on = period * duty / 100
            points += [(start, high), (start + on, high), (start + on, base),
                       (start + period, base)]
        parts.append(path(points, PUSH, 4))
        level = base - 62 * duty / 100
        parts.append(line(x0, level, x0 + cycles * period, level, MOTION, 3, "10 8"))
    # One cycle, marked under the last signal.
    y = 124 + 2 * 158 + 22
    parts.append(arrow(x0, y, x0 + period, y, MUTED, 3, both=True))
    parts.append(text(x0 + period / 2, y + 26, "one cycle", 18, MUTED))
    parts.append(line(330, y + 19, 378, y + 19, MOTION, 3, "10 8"))
    parts.append(text(388, y + 26, "average power", 18, INK, anchor="start"))
    svg("pwm_duty.svg", 496, "PWM: a pin switched on for 25%, 50% and 75% of each cycle", parts)


def encoder_ab():
    """A and B a quarter cycle apart, as encoders.cpp counts them: when A changes, A != B counts
    +1; when B changes, A == B counts +1. Both panels start with A and B low."""
    q, x0 = 62, 86  # q = a quarter of a cycle, on screen

    def panel(top, title, first, sign):
        """first = the signal that changes first ("A" or "B")."""
        parts = [text(40, top, title, 20, INK, "bold", "start")]
        levels = {"A": top + 72, "B": top + 146}
        height = 44
        for name, base in levels.items():
            lead = name == first
            # The leading signal rises at step 1, the other at step 2; each stays high for two.
            rise = 1 if lead else 2
            points = [(x0, base)]
            for step in range(rise, 9, 4):
                on, off = x0 + (step - 0.5) * q, x0 + (step + 1.5) * q
                points += [(on, base), (on, base - height), (off, base - height), (off, base)]
            points.append((x0 + 8 * q, base))
            parts.append(path(points, PUSH if name == "A" else "#2b8a3e", 4))
            parts.append(text(58, base - 14, name, 24, INK, "bold"))
        for step in range(1, 9):  # a tick and a count at every change, on A or on B
            x = x0 + (step - 0.5) * q
            parts.insert(1, line(x, top + 16, x, top + 152, CANCEL, 2, "4 6"))
            parts.append(text(x, top + 178, f"{'+' if sign > 0 else '−'}1", 19, MOTION, "bold"))
        return parts

    parts = panel(38, "One direction: A changes first; each change counts +1", "A", 1)
    # One pulse is one full cycle of a signal: four changes.
    y = 38 + 200
    x1, x2 = x0 + 0.5 * q, x0 + 4.5 * q
    parts.append(arrow(x1, y, x2, y, MUTED, 3, both=True))
    parts.append(text((x1 + x2) / 2, y + 26, "1 pulse = 4 counts", 19, INK, "bold"))
    parts += panel(322, "Other direction: B changes first; each change counts −1", "B", -1)
    svg("encoder_ab.svg", 526, "The encoder's two signals, A and B, a quarter cycle apart, with "
        "each change counted", parts)


def wheel_push_direction():
    """How to read a wheel's push. Stand outside the robot and face the wheel: one whose top moves
    to your right rolls to your right, which turns the robot anticlockwise. The rule is the same
    at every wheel, so the robot is drawn with no front mark and the wheel nearest to you."""
    parts = []
    for k, (title, setting, sign) in enumerate((
            ("Pushes anticlockwise", "flipMotor false", 1),
            ("Pushes clockwise", "flipMotor true", -1))):
        top = 16 + k * 322
        parts.append(text(40, top + 32, title, 24, INK, "bold", "start"))
        parts.append(text(600, top + 32, setting, 22, MOTION, "bold", "end"))
        # From above: the robot and the wheel being read.
        cx, cy, r = 150, top + 176, 74
        parts.append(f'<circle cx="{cx}" cy="{cy}" r="{r - 8}" fill="{BODY}" stroke="{BODY_EDGE}" '
                     f'stroke-width="3"/>')
        wx, wy = cx, cy + r
        parts.append(wheel(wx, wy, 0, 60, 22))
        parts.append(arrow(wx + sign * 36, wy, wx + sign * 104, wy, PUSH, 6))
        start, end = (20, 100) if sign > 0 else (-20, -100)
        parts.append(curved_arrow(cx, cy, 36, start, end, MOTION, 5))
        parts.append(text(cx, top + 60, "from above", 18, MUTED))
        # An eye mark: where you stand for the view on the right.
        parts.append(text(wx, wy + 48, "you", 18, INK))
        parts.append(arrow(wx, wy + 30, wx, wy + 16, MUTED, 3))
        # From outside: the wheel face on, on the floor.
        fx, fy, fr = 450, top + 182, 72
        parts.append(line(fx - 120, fy + fr + 12, fx + 120, fy + fr + 12, CARD_EDGE, 4))
        for n in range(10):
            a = n * 36 + 18
            x = fx + fr * math.cos(math.radians(a))
            y = fy + fr * math.sin(math.radians(a))
            parts.append(f'<g transform="translate({x:.1f} {y:.1f}) rotate({a + 90})">'
                         f'<rect x="-17" y="-9" width="34" height="18" rx="9" fill="{WHEEL}"/>'
                         f'</g>')
        parts.append(f'<circle cx="{fx}" cy="{fy}" r="{fr - 12}" fill="#e4e8ed" stroke="{WHEEL}" '
                     f'stroke-width="3"/>')
        parts.append(f'<circle cx="{fx}" cy="{fy}" r="14" fill="{WHEEL}"/>')
        parts.append(arrow(fx - sign * 56, fy - fr - 22, fx + sign * 56, fy - fr - 22, PUSH, 6))
        parts.append(text(fx, top + 60, "from outside the robot", 18, MUTED))
        word = "right" if sign > 0 else "left"
        parts.append(text(fx, fy + fr + 42, f"The top moves to your {word}.", 19, INK))
    parts.append(text(320, 672, "The rule is the same for every wheel: stand outside the robot and "
                               "face that wheel.", 17, INK))
    svg("push_direction.svg", 694, "A wheel whose top moves to your right pushes the robot "
        "anticlockwise", parts)


# ------------------------------------------------------------------------------------
#  Wiring. The DRV8876 drawing's colours (W_*) show each wire's job; the wiring diagram's
#  (C_*) are those of the real motor cable, as photographed on the robot.
# ------------------------------------------------------------------------------------

W_PLUS = "#c92a2a"     # battery + and Motor+
W_MINUS = "#212529"    # battery -, GND and Motor-
W_A = "#1c6fd6"        # encoder A, and the speed signal
W_B = "#2b8a3e"        # encoder B, and the direction signal
W_5V = "#7048e8"       # 5V to SLEEP

C_RED = "#d92c2c"      # Motor+, battery +
C_WHITE = "#ffffff"    # Motor-
C_BLACK = "#212529"    # Enc GND, battery -
C_GREEN = "#2b8a3e"    # Enc A
C_YELLOW = "#f5c400"   # Enc B
C_BLUE = "#1c6fd6"     # Enc +
PALE_WIRES = (C_WHITE, C_YELLOW)  # these get a thin dark outline, to show on the white card


def drv8876_one_motor():
    """One Pololu #4036 board, top view with its pins in their real order."""
    left_pins = ("VM", "GND", "EN/IN1", "PH/IN2", "PMODE", "SLEEP", "VREF")
    right_pins = ("VIN", "GND", "OUT1", "OUT2", "IMODE", "FAULT", "CS")
    used = {"GND", "EN/IN1", "PH/IN2", "PMODE", "SLEEP", "VIN", "OUT1", "OUT2"}
    bx, bw, y0, step = 232, 176, 128, 52
    parts = [box(bx, y0 - 34, bw, step * 6 + 68, "#e6efe3", "#7f9a79"),
             text(bx + bw / 2, y0 - 50, "Pololu DRV8876 (#4036)", 20, INK, "bold")]
    ly = {name: y0 + n * step for n, name in enumerate(left_pins)}
    ry = {name: y0 + n * step for n, name in enumerate(right_pins)}

    # The controller board. Its pins are drawn level with the driver's, so no wire crosses
    # another; PMODE simply takes a second GND pin (controller boards have several).
    parts.append(box(22, ly["GND"] - 34, 128, ly["SLEEP"] - ly["GND"] + 68))
    parts.append(text(86, ly["GND"] - 46, "Controller", 19, INK, "bold"))
    wires = (("GND", "GND", W_MINUS), ("Speed", "EN/IN1", W_A), ("Direction", "PH/IN2", W_B),
             ("GND", "PMODE", W_MINUS), ("5V", "SLEEP", W_5V))
    for name, pin, colour in wires:
        y = ly[pin]
        parts.append(text(40, y + 6, name, 17, INK, anchor="start"))
        parts.append(path([(150, y), (bx, y)], colour))
        parts.append(dot(150, y, 7, CARD, INK))

    # The battery, through a switch, and the motor.
    parts.append(box(486, 66, 132, 92))
    parts.append(text(552, 100, "Battery", 19, INK, "bold"))
    parts.append(text(552, 124, "12 V", 18, MUTED))
    parts.append(text(474, 118, "+", 22, W_PLUS, "bold"))
    parts.append(text(482, 174, "−", 24, W_MINUS, "bold"))
    vin_y, g_y = ry["VIN"], ry["GND"]
    parts.append(path([(bx + bw, vin_y), (486, vin_y)], W_PLUS))
    parts.append(f'<rect x="424" y="{vin_y - 11}" width="28" height="22" rx="4" fill="{CARD}" '
                 f'stroke="{INK}" stroke-width="2.5"/>')
    parts.append(text(438, vin_y - 18, "switch", 16, INK))
    parts.append(path([(bx + bw, g_y), (470, g_y), (470, 146), (486, 146)], W_MINUS))
    parts.append(path([(bx + bw, ry["OUT1"]), (520, ry["OUT1"])], W_PLUS))
    parts.append(path([(bx + bw, ry["OUT2"]), (520, ry["OUT2"])], W_MINUS))
    parts.append(f'<circle cx="548" cy="{ry["OUT1"] + 26}" r="42" fill="{BODY}" stroke="{INK}" '
                 f'stroke-width="3"/>')
    parts.append(text(548, ry["OUT1"] + 36, "M", 30, INK, "bold"))
    parts.append(text(460, ry["OUT1"] - 10, "Motor+", 17, INK))
    parts.append(text(460, ry["OUT2"] + 24, "Motor−", 17, INK))

    # The pins, on top of the wires.
    for names, ys, x, anchor, shift in ((left_pins, ly, bx, "start", 16),
                                        (right_pins, ry, bx + bw, "end", -16)):
        for name in names:
            on = name in used
            parts.append(dot(x, ys[name], 8, CARD if on else "#d9ded9", INK if on else CANCEL))
            parts.append(text(x + shift, ys[name] + 6, name, 17, INK if on else MUTED,
                              "bold" if on else "normal", anchor))
    parts.append(text(320, 500, "Grey pins stay unconnected.", 18, MUTED))
    parts.append(text(320, 526, "Wire all three driver boards the same way, each to its own motor "
                                "and control pins.", 17, MUTED))
    svg("drv8876_one_motor.svg", 548, "One Pololu DRV8876 board wired to a motor, the battery and "
        "the controller board", parts)


def wiring_diagram():
    """The Romeo held as in DFRobot's On-board Function Diagram (USB-C on the left). Socket
    polarity and the encoder header's layout are as printed on the board: M1 and M3 read + -,
    M4 reads - +, VM and VIN have - above +, and the header's rows are IO, 3V3, GND from the
    inside out. The battery lead splits into two pairs, one for VM and one for VIN."""
    W = 960
    parts = []
    bx, by, bw, bh = 250, 170, 560, 300

    def wire(points, colour, width=4):
        if colour in PALE_WIRES:
            parts.append(path(points, INK, width + 1.8))
        parts.append(path(points, colour, width))

    # ---- The board ----
    parts.append(box(bx, by, bw, bh, "#2f3640", "#2f3640", 16))
    chip = "#49525e"
    label = "#e8ecf1"
    parts.append(text(bx + 250, by + 150, "Romeo ESP32-S3", 24, label, "bold"))

    # M2 (unused) and M4 on the top edge.
    m4 = {"−": 580, "+": 628}
    parts.append(box(430, by, 88, 34, "#59625e", "#59625e", 4, 2))
    parts.append(text(474, by + 56, "M2 (unused)", 16, "#aeb6bf"))
    parts.append(box(560, by, 88, 34, "#7bb369", "#5d8f4e", 4, 2))
    parts.append(text(604, by + 56, "− M4 +", 20, label, "bold"))
    # The module and the antenna.
    parts.append(box(672, by + 18, 112, 100, chip, "#737d8a", 4, 2))
    parts.append(text(728, by + 62, "ESP32-S3", 17, label))
    parts.append(dot(766, by + 102, 6, "#d9b44a"))
    parts.append(path([(766, by + 102), (838, by + 102), (838, by + 40)], MUTED, 3))
    parts.append(text(818, by + 28, "Antenna", 18, INK, anchor="start"))
    # USB-C, left edge.
    parts.append(box(bx - 8, by + 104, 44, 40, "#b8c0c9", "#8c96a3", 4, 2))
    parts.append(text(bx + 44, by + 130, "USB-C", 17, label, anchor="start"))
    # VIN and VM, right edge.
    parts.append(box(bx + bw - 34, by + 118, 42, 72, "#7bb369", "#5d8f4e", 4, 2))
    parts.append(text(bx + bw - 44, by + 160, "VIN", 20, label, "bold", "end"))
    vin_minus, vin_plus = by + 132, by + 174
    vm_minus, vm_plus = by + 210, by + 252
    parts.append(box(bx + bw - 34, by + 196, 42, 72, "#7bb369", "#5d8f4e", 4, 2))
    parts.append(text(bx + bw - 44, by + 238, "VM", 20, label, "bold", "end"))
    # JP1, and SW on the bottom edge.
    parts.append(box(bx + bw - 128, by + 196, 22, 40, "#111418", "#d9dee4", 3, 2))
    parts.append(text(bx + bw - 117, by + 188, "JP1", 16, label))
    parts.append(box(bx + bw - 118, by + bh - 40, 74, 28, "#d9dee4", "#d9dee4", 3, 2))
    parts.append(text(bx + bw - 81, by + bh - 20, "SW", 17, INK, "bold"))
    # M1 and M3 on the bottom edge.
    m1 = {"+": 490, "−": 538}
    m3 = {"+": 610, "−": 658}
    for x, name in ((466, "+ M1 −"), (586, "+ M3 −")):
        parts.append(box(x, by + bh - 34, 96, 34, "#7bb369", "#5d8f4e", 4, 2))
        parts.append(text(x + 48, by + bh - 44, name, 20, label, "bold"))
    # Jumpers: PMODE above M1EN and M1PH.
    jx = 402
    parts.append(box(jx, by + 174, 22, 40, "#111418", "#d9dee4", 3, 2))
    parts.append(text(jx + 11, by + 166, "PMODE", 16, label))
    for x, name in ((384, "M1EN"), (432, "M1PH")):
        parts.append(box(x, by + bh - 62, 22, 40, "#111418", "#d9dee4", 3, 2))
        parts.append(text(x + 11, by + bh - 8, name, 14, label))
    # The encoder header's place on the board.
    hx, hy, hw, hh = bx + 16, by + bh - 62, 104, 50
    parts.append(box(hx, hy, hw, hh, "#3f7d43", label, 3, 2))
    parts.append(text(hx + hw / 2, hy - 9, "Encoder header", 15, label))

    # ---- The encoder header, enlarged ----
    px, py, pw, ph = 92, 535, 380, 155
    parts.append(line(hx, hy + hh, px, py, MUTED, 2, "6 6"))
    parts.append(line(hx + hw, hy + hh, px + pw, py, MUTED, 2, "6 6"))
    parts.append(box(px, py, pw, ph, CARD, BODY_EDGE, 8, 2.5))
    cols = {"IO17": 150, "IO16": 199, "IO15": 248, "IO7": 297, "IO6": 346, "IO5": 395, "IO4": 444}
    rows = {"IO": 590, "3V3": 625, "GND": 660}
    for name, x in cols.items():
        parts.append(text(x, 565, name, 16, INK, "bold"))
    for name, y in rows.items():
        parts.append(text(px + 6, y + 6, name, 15, INK, "bold", "start"))

    # ---- The motors ----
    def motor(x, y, w, h, name, where):
        parts.append(box(x, y, w, h, BODY, BODY_EDGE))
        parts.append(text(x + w / 2, y + h / 2 - 2, name, 20, INK, "bold"))
        parts.append(text(x + w / 2, y + h / 2 + 22, where, 16, MUTED))

    off = 13  # a wire to a pin runs just beside its column, then steps across to the pin

    def encoder(x_a, x_b, routes):
        """Connects one motor's encoder: A and + in x_a's column, B and GND in x_b's. routes
        gives, for each wire, the points up to the bottom of its vertical run."""
        a_v, p_v, g_v, b_v = x_a - off, x_a + off, x_b, x_b - off
        runs = {"A": (a_v, rows["IO"], x_a, C_GREEN), "+": (p_v, rows["3V3"], x_a, C_BLUE),
                "GND": (g_v, rows["GND"], x_b, C_BLACK), "B": (b_v, rows["IO"], x_b, C_YELLOW)}
        for key, (vx, ry, pin_x, colour) in runs.items():
            wire(routes[key] + [(vx, routes[key][-1][1]), (vx, ry), (pin_x, ry)], colour, 3.5)
            parts.append(dot(pin_x, ry, 6, colour, INK if colour in PALE_WIRES else None))

    # M1: straight up to its socket; encoder out of the left side.
    motor(465, 745, 110, 80, "M1 motor", "front left, 60°")
    wire([(m1["+"], 745), (m1["+"], by + bh - 17)], C_RED)
    wire([(m1["−"], 745), (m1["−"], by + bh - 17)], C_WHITE)
    encoder(cols["IO4"], cols["IO5"], {"+": [(465, 760)], "A": [(465, 775)], "GND": [(465, 790)],
                                       "B": [(465, 805)]})
    # M3: up to its socket; encoder out of the bottom and under M1.
    motor(590, 745, 110, 80, "M3 motor", "back, 180°")
    wire([(m3["+"], 745), (m3["+"], by + bh - 17)], C_RED)
    wire([(m3["−"], 745), (m3["−"], by + bh - 17)], C_WHITE)
    encoder(cols["IO6"], cols["IO7"], {"+": [(610, 825), (610, 842)], "A": [(630, 825), (630, 857)],
                                       "GND": [(650, 825), (650, 872)],
                                       "B": [(670, 825), (670, 887)]})
    # M4: down to its socket; encoder round the left of the board.
    motor(545, 44, 118, 80, "M4 motor", "front right, 300°")
    wire([(m4["−"], 124), (m4["−"], by + 17)], C_WHITE)
    wire([(m4["+"], 124), (m4["+"], by + 17)], C_RED)
    encoder(cols["IO15"], cols["IO16"], {
        "+": [(545, 58), (30, 58), (30, 754)], "A": [(545, 74), (46, 74), (46, 738)],
        "GND": [(545, 90), (62, 90), (62, 722)], "B": [(545, 106), (78, 106), (78, 706)]})
    # The terminals, drawn over the wire ends.
    for x in (*m1.values(), *m3.values()):
        parts.append(dot(x, by + bh - 17, 6, "#eef3ea", "#3c5f33"))
    for x in m4.values():
        parts.append(dot(x, by + 17, 6, "#eef3ea", "#3c5f33"))

    # Empty pins, on top of the wires.
    taken = {("IO4", "IO"), ("IO4", "3V3"), ("IO5", "IO"), ("IO5", "GND"), ("IO6", "IO"),
             ("IO6", "3V3"), ("IO7", "IO"), ("IO7", "GND"), ("IO15", "IO"), ("IO15", "3V3"),
             ("IO16", "IO"), ("IO16", "GND")}
    for cname, x in cols.items():
        for rname, y in rows.items():
            if (cname, rname) not in taken:
                parts.append(dot(x, y, 6, CARD, CANCEL))

    # ---- The battery ----
    tx = bx + bw - 13
    parts.append(box(842, by + 202, 100, 78))
    parts.append(text(892, by + 236, "Battery", 18, INK, "bold"))
    parts.append(text(892, by + 260, "12 V", 17, MUTED))
    wire([(tx, vm_minus), (842, vm_minus)], C_BLACK)
    wire([(tx, vm_plus), (842, vm_plus)], C_RED)
    wire([(tx, vin_minus), (918, vin_minus), (918, by + 202)], C_BLACK)
    wire([(tx, vin_plus), (880, vin_plus), (880, by + 202)], C_RED)
    parts.append(text(830, vm_minus - 7, "−", 22, C_BLACK, "bold"))
    parts.append(text(830, vm_plus - 7, "+", 20, C_RED, "bold"))
    parts.append(text(830, vin_minus - 7, "−", 22, C_BLACK, "bold"))
    parts.append(text(830, vin_plus - 7, "+", 20, C_RED, "bold"))
    for y in (vin_minus, vin_plus, vm_minus, vm_plus):
        parts.append(dot(tx, y, 6, "#eef3ea", "#3c5f33"))

    # ---- The key ----
    kx, ky = 722, 548
    parts.append(text(kx, ky, "Wire colours", 17, INK, "bold", "start"))
    for n, (colour, words) in enumerate(((C_RED, "Motor+, battery +"), (C_WHITE, "Motor−"),
                                         (C_BLACK, "Enc GND, battery −"), (C_GREEN, "Enc A"),
                                         (C_YELLOW, "Enc B"), (C_BLUE, "Enc + (3V3)"))):
        y = ky + 26 + n * 25
        wire([(kx, y - 5), (kx + 34, y - 5)], colour, 5)
        parts.append(text(kx + 44, y, words, 17, INK, anchor="start"))
    parts.append(text(kx, ky + 192, "Not to scale. PMODE, M1EN,", 16, MUTED, anchor="start"))
    parts.append(text(kx, ky + 212, "M1PH and JP1 are fitted.", 16, MUTED, anchor="start"))
    parts.append(text(kx, ky + 232, "IO17 stays free.", 16, MUTED, anchor="start"))
    svg("wiring_diagram.svg", 912, "Wiring diagram: the Romeo ESP32-S3, three motors and the battery",
        parts, width=W)


def main():
    omni_wheel_directions()
    wheel_angles_top_view()
    kiwi_layout_top()
    move_shadow()
    kiwi_forward()
    kiwi_slide_left()
    drive_commands_top_view()
    h_bridge()
    pwm_duty()
    encoder_ab()
    wheel_push_direction()
    drv8876_one_motor()
    wiring_diagram()


if __name__ == "__main__":
    main()
