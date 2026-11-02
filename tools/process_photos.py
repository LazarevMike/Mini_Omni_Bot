"""Turns the original photos of the robot into the pictures used by the docs.

Reads docs/images/originals/IMG_*.JPG and writes docs/images/photos/IMG_*.jpg. The originals
are never changed. The output is deterministic: the same originals always give the same files.

The full-size originals are not in the repository. To re-make the pictures, place them locally
in docs/images/originals/; that folder is ignored by git. The processed pictures in
docs/images/ are in the repository, so the docs need neither this script nor the originals.

Run it from anywhere:

    python3 tools/process_photos.py               # every photo the docs use
    python3 tools/process_photos.py 6934 6938     # only these numbers
    python3 tools/process_photos.py --debug DIR   # also write the desk masks to DIR

It needs numpy, OpenCV and Pillow. Nothing else in the project uses these libraries: they are
only for re-making the pictures, not for building or running the robot.

Each photo goes through these steps:

    1. Colour: apply the EXIF orientation, convert Display P3 to sRGB.
    2. Framing: optional straightening and crop from the SETTINGS table. A few crop boxes run
       past the edge of the photo, to even up a tight margin: that adds bare desk, nothing else.
    3. Calibration: neutral white balance and a common exposure, measured on the light desk.
    4. Desk: find the desk, measure its lighting, and divide that lighting away, so that uneven
       light and soft cast shadows flatten to one clean near-white. A faint contact shadow is
       kept right next to the object. The robot is outside this mask and is not altered.
    5. Subject: gentle tone curve, natural saturation, a little local contrast and sharpening.
    6. Output: sRGB JPEG, no metadata, longest side 1600 pixels, about 350 KB at most.

Close-ups with little desk in view use mode "closeup": steps 3 and 5 only, more gently, and no
step 4. Nothing here paints, removes or invents any part of the robot: step 4 only changes
pixels that it measured as bare desk.
"""

import argparse
import io
import multiprocessing
import os
import sys

import cv2
import numpy as np
from PIL import Image, ImageCms, ImageOps

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
IN_DIR = os.path.join(ROOT, "docs", "images", "originals")
OUT_DIR = os.path.join(ROOT, "docs", "images", "photos")

# Photos that no page uses: made only when asked for by number, and not kept in the repository.
NOT_USED = {"6940", "6947", "6951", "6956", "6957"}

LONGEST_SIDE = 1600
MAX_BYTES = 350_000
JPEG_QUALITY = 85
JPEG_QUALITY_MIN = 78

# The desk's level in the finished picture, in linear light: sRGB 242 is a clean near-white.
DESK_TARGET = 0.887

# Defaults for every photo. Any of these can be overridden per photo in SETTINGS.
DEFAULTS = {
    "mode": "desk",        # "desk": flatten the desk; "closeup": gentle treatment only
    "rotate": 0.0,         # degrees, positive turns the picture anticlockwise
    "crop": None,          # (x0, y0, x1, y1) in pixels of the upright original; a box that
                           # runs past the edge of the photo extends the bare desk there
    "keep": (),            # polygons (fractions of the cropped frame) that are never desk
    "shadow_areas": (),    # polygons where a dark, sharp-edged shadow lies on the desk
    "loose_wires": False,  # also clean the desk between loose wires, away from solid parts
    "bleed_limit": 28.0,   # colour above this on bright desk counts as a part, not bounced light
    "exposure": 1.0,       # extra multiplier on the subject's exposure
    "max_gain": 1.7,       # upper limit of the exposure gain on the subject
    "shadow_keep": 0.45,   # how much of the shadow stays next to the object, 0 to 1
    "shadow_reach": 34.0,  # pixels over which that contact shadow fades out
    "texture": 0.55,       # how much of the desk's fine dark texture stays, 0 to 1
    "bright_texture": 0.3, # the same for what is brighter than the desk around it, 0 to 1
    "saturation": 1.12,    # colour gain on the subject
    "contrast": 0.16,      # strength of the tone curve on the subject
    "clarity": 0.16,       # local contrast on the subject
    "sharpen": 0.55,       # fine sharpening on the subject
}

CLOSEUP = {
    "mode": "closeup",
    "max_gain": 1.35,
    "saturation": 1.08,
    "contrast": 0.10,
    "clarity": 0.10,
    "sharpen": 0.45,
}

SETTINGS = {
    "6934": dict(crop=(380, 400, 3620, 2830)),
    "6936": dict(crop=(0, 440, 3024, 3640)),
    "6937": dict(CLOSEUP),
    "6938": dict(rotate=1.2, crop=(360, 735, 2640, 3295),
                 keep=[[(.27, .17), (.44, .17), (.44, .36), (.27, .36)]]),
    "6939": dict(crop=(-110, 500, 2880, 3900)),
    "6940": dict(rotate=-3.2, crop=(420, 200, 3930, 2830),
                 shadow_areas=[[(.544, .608), (.60, .608), (.60, .803), (.555, .803)]]),
    "6941": dict(crop=(0, 60, 3940, 3024), texture=0.15, bright_texture=0.05, shadow_keep=0.2,
                 keep=[[(.10, .58), (.33, .58), (.33, .84), (.10, .84)]],
                 shadow_areas=[[(.350, .45), (.44, .45), (.44, 1.0), (.342, 1.0)],
                               [(.7825, .3314), (.828, .3256), (.828, .382), (.806, .382)]]),
    "6942": dict(crop=(-100, 0, 3010, 3940), loose_wires=True),
    "6943": dict(crop=(-190, 200, 3024, 3620), loose_wires=True),
    "6944": dict(crop=(771, 1300, 1914, 2725), shadow_keep=0.25,
                 keep=[[(.43, .145), (.585, .125), (.70, .14), (.70, .28), (.43, .28)]],
                 shadow_areas=[[(.376, .563), (.935, .563), (.935, 1.0), (.376, 1.0)]]),
    "6945": dict(crop=(440, 540, 3176, 2592), bleed_limit=60.0),
    "6946": dict(crop=(0, 0, 3600, 3024), loose_wires=True),
    "6947": dict(crop=(380, 0, 3880, 3024), loose_wires=True),
    "6948": dict(crop=(300, -100, 3660, 3124), loose_wires=True),
    "6949": dict(CLOSEUP, exposure=0.9, clarity=0.05, sharpen=0.3),
    "6950": dict(crop=(-100, 210, 2990, 3430), loose_wires=True),
    "6951": dict(crop=(0, 240, 3024, 3680), loose_wires=True,
                 shadow_areas=[[(.281, .681), (.313, .684), (.334, .725), (.377, .759), (.434, .788),
                                (.466, .791), (.455, .831), (.398, .856), (.284, .856), (.235, .813),
                                (.242, .738)]]),
    "6952": dict(CLOSEUP, crop=(0, 120, 3880, 3024)),
    "6953": dict(CLOSEUP),
    "6954": dict(CLOSEUP, crop=(0, 520, 4032, 3024)),
    "6955": dict(crop=(150, 0, 4032, 2520),
                 keep=[[(.66, .15), (.81, .15), (.81, .42), (.66, .42)]]),
    "6956": dict(crop=(735, 693, 3335, 2643), exposure=1.15, max_gain=1.9),
    "6957": dict(crop=(-70, 260, 3024, 4064), loose_wires=True),
    "6958": dict(CLOSEUP),
    "6959": dict(CLOSEUP, crop=(0, 130, 3300, 3024)),
    "6960": dict(CLOSEUP, crop=(0, 350, 4032, 3024)),
    "6961": dict(crop=(570, 0, 3594, 2830)),
    "6962": dict(crop=(620, 0, 3390, 2790)),
    "6963": dict(crop=(640, 0, 3630, 3024),
                 keep=[[(.27, .18), (.38, .18), (.38, .29), (.27, .29)]]),
}


def srgb_to_linear(x):
    return np.where(x <= 0.04045, x / 12.92, ((x + 0.055) / 1.055) ** 2.4).astype(np.float32)


def linear_to_srgb(x):
    x = np.clip(x, 0.0, 1.0)
    return np.where(x <= 0.0031308, x * 12.92, 1.055 * x ** (1 / 2.4) - 0.055).astype(np.float32)


def luminance(lin):
    return lin[..., 0] * 0.2126 + lin[..., 1] * 0.7152 + lin[..., 2] * 0.0722


def disk(radius):
    return cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (2 * radius + 1, 2 * radius + 1))


def load_upright_srgb(path):
    """Opens a photo, applies its EXIF orientation and converts it to sRGB."""
    image = Image.open(path)
    icc = image.info.get("icc_profile")
    image = ImageOps.exif_transpose(image).convert("RGB")
    if icc:
        source = ImageCms.ImageCmsProfile(io.BytesIO(icc))
        target = ImageCms.createProfile("sRGB")
        image = ImageCms.profileToProfile(
            image, source, target,
            renderingIntent=ImageCms.Intent.RELATIVE_COLORIMETRIC, outputMode="RGB")
    return image


def frame(image, settings):
    """Applies the straightening, crop and resize.

    Returns linear RGB as float32, and how many pixels of desk to add on each side
    (left, top, right, bottom) where the crop box runs past the edge of the photo.
    """
    if settings["rotate"]:
        image = image.rotate(settings["rotate"], resample=Image.BICUBIC, fillcolor=None)
    x0, y0, x1, y1 = settings["crop"] or (0, 0, image.width, image.height)
    scale = min(1.0, LONGEST_SIDE / max(x1 - x0, y1 - y0))
    pad = [round(v * scale) for v in
           (max(0, -x0), max(0, -y0), max(0, x1 - image.width), max(0, y1 - image.height))]
    image = image.crop((max(0, x0), max(0, y0), min(image.width, x1), min(image.height, y1)))
    if scale < 1.0:
        size = (round(image.width * scale), round(image.height * scale))
        image = image.resize(size, Image.LANCZOS)
    return srgb_to_linear(np.asarray(image, dtype=np.float32) / 255.0), pad


def extend_desk(out, desk, pad):
    """Adds bare desk around a finished picture, mirrored from desk pixels only."""
    left, top, right, bottom = pad
    if not any(pad):
        return out
    fill = np.where(desk[..., None], out, DESK_TARGET)
    bigger = cv2.copyMakeBorder(fill, top, bottom, left, right, cv2.BORDER_REFLECT_101)
    h, w = out.shape[:2]
    # A shadow in the mirrored part fades out quickly, so that no second object is suggested.
    inside = np.zeros(bigger.shape[:2], np.uint8)
    inside[top:top + h, left:left + w] = 1
    distance = cv2.distanceTransform(1 - inside, cv2.DIST_L2, 3)
    floor = DESK_TARGET * (0.55 + 0.40 * np.minimum(distance / 20.0, 1.0))
    bigger = np.maximum(bigger, floor[..., None])
    bigger[top:top + h, left:left + w] = out
    return bigger


def to_lab(lin):
    return cv2.cvtColor(linear_to_srgb(lin), cv2.COLOR_RGB2LAB)


def smooth_fill(values, weights, sigmas=(4.0, 12.0, 36.0, 108.0, 324.0)):
    """Normalised convolution: a smooth field through `values` where `weights` is set.

    Fine scales are used where there is enough desk nearby, coarser ones fill the rest.
    """
    h, w = weights.shape
    result = None
    for sigma in reversed(sigmas):
        small = max(1.0, sigma / 4.0)
        size = (max(8, int(w / small)), max(8, int(h / small)))
        s_w = cv2.resize(weights, size, interpolation=cv2.INTER_AREA)
        s_v = cv2.resize(values * (weights[..., None] if values.ndim == 3 else weights),
                         size, interpolation=cv2.INTER_AREA)
        s = sigma / small
        s_w = cv2.GaussianBlur(s_w, (0, 0), s)
        s_v = cv2.GaussianBlur(s_v, (0, 0), s)
        s_w_up = cv2.resize(s_w, (w, h), interpolation=cv2.INTER_LINEAR)
        s_v_up = cv2.resize(s_v, (w, h), interpolation=cv2.INTER_LINEAR)
        if values.ndim == 3:
            field = s_v_up / np.maximum(s_w_up[..., None], 1e-5)
        else:
            field = s_v_up / np.maximum(s_w_up, 1e-5)
        confidence = np.clip((s_w_up - 0.02) / 0.10, 0.0, 1.0)
        if result is None:
            result = field
        else:
            c = confidence[..., None] if values.ndim == 3 else confidence
            result = result * (1 - c) + field * c
    return result


def polygon_mask(shape, polygons):
    h, w = shape
    mask = np.zeros((h, w), np.uint8)
    for polygon in polygons:
        points = np.array([[x * w, y * h] for x, y in polygon], np.int32)
        cv2.fillPoly(mask, [points], 1)
    return mask.astype(bool)


def measure_white(lin):
    """Finds the desk's white in linear RGB: the bright, colourless part of the picture."""
    lab = to_lab(lin)
    a, b = lab[..., 1], lab[..., 2]
    light = lab[..., 0]
    top = np.percentile(light, 92)
    neutral = (np.abs(a) < 10) & (np.abs(b) < 12) & (light > top * 0.72) & (light < 99.5)
    if neutral.sum() < 2000:
        return None
    lum = luminance(lin)[neutral]
    return np.median(lin[neutral][lum >= np.percentile(lum, 55)], axis=0)


def find_desk(lin_norm, settings):
    """Returns a boolean mask of bare desk, from a picture with the desk near DESK_TARGET."""
    h, w = lin_norm.shape[:2]
    lab = to_lab(lin_norm)
    light = lab[..., 0]
    chroma = np.hypot(lab[..., 1], lab[..., 2])
    keep = polygon_mask((h, w), settings["keep"])

    blurred = cv2.GaussianBlur(light, (0, 0), 1.2)
    gx = cv2.Sobel(blurred, cv2.CV_32F, 1, 0, ksize=3) / 8
    gy = cv2.Sobel(blurred, cv2.CV_32F, 0, 1, ksize=3) / 8
    gradient = np.hypot(gx, gy)

    # Pixels that cannot be desk: dark, coloured, or on a sharp edge.
    # Pale colour on bright desk is light bounced off a part, so it needs a higher limit.
    soft_chroma = cv2.GaussianBlur(chroma, (0, 0), 2.0)
    solid = (light < 48.0) | np.where(light > 62.0, soft_chroma > settings["bleed_limit"],
                                      chroma > 13.0)
    evidence = solid | (gradient > 2.6) | keep

    # Inside a marked shadow only what is truly black or coloured counts as a part.
    shadow = polygon_mask((h, w), settings["shadow_areas"])
    evidence[shadow] = (light < 30.0)[shadow] | (chroma > 13.0)[shadow]

    # Solid bodies, without the thin wires: the plates, the board, the wheels.
    mass = cv2.morphologyEx(evidence.astype(np.uint8), cv2.MORPH_OPEN, disk(5))
    mass = cv2.dilate(mass, disk(6)).astype(bool)

    # Everything else is a candidate. Large pieces are the desk itself. A small piece counts
    # as desk only if it is not hemmed in by a solid body: that keeps a hole in a plate or a
    # highlight on a part out, and lets the desk inside a loop of wire in.
    candidate = ~cv2.dilate(evidence.astype(np.uint8), disk(2)).astype(bool)
    count, labels, stats, _ = cv2.connectedComponentsWithStats(
        candidate.astype(np.uint8), connectivity=4)
    edge = candidate & ~cv2.erode(candidate.astype(np.uint8), disk(1)).astype(bool)
    edge_all = np.bincount(labels[edge], minlength=count).astype(np.float32)
    edge_mass = np.bincount(labels[edge & mass], minlength=count).astype(np.float32)
    hemmed = edge_mass / np.maximum(edge_all, 1.0)
    areas = stats[:, cv2.CC_STAT_AREA]
    busy = cv2.blur(evidence.astype(np.float32), (61, 61))
    density = np.zeros(count, np.float32)
    np.add.at(density, labels.ravel(), busy.ravel())
    density /= np.maximum(areas, 1)
    accepted = (areas >= 0.03 * h * w) | ((areas >= 4000) & (density < 0.24))
    accepted |= (hemmed < 0.65) & (density < 0.24)
    accepted &= areas >= 12
    accepted[0] = False
    desk = accepted[labels]

    # What is left and shows no real difference from the desk is desk too: scuffs and specks
    # on the surface. A real part differs strongly over many pixels.
    log_light = np.log(np.maximum(luminance(lin_norm), 1e-3))
    field = smooth_fill(log_light, desk.astype(np.float32))
    strong = (np.abs(log_light - field) > 0.35) | (chroma > 22.0)
    rest = (~desk).astype(np.uint8)
    count, labels, stats, _ = cv2.connectedComponentsWithStats(rest, connectivity=8)
    strong_count = np.bincount(labels[strong], minlength=count)
    faint = strong_count < 30
    # Small, colourless and never truly dark: dust and dirt on the desk.
    coloured = np.bincount(labels[chroma > 22.0], minlength=count)
    darkest = np.full(count, 100.0, np.float32)
    np.minimum.at(darkest, labels.ravel(), light.ravel())
    faint |= (stats[:, cv2.CC_STAT_AREA] < 1500) & (coloured < 12) & (darkest > 30.0)
    faint[0] = False
    desk |= faint[labels]

    # Among loose wires the shadows are thin and close together. There, everything that is
    # outside the solid parts, and is neither coloured, dark nor a sharp edge, is desk.
    if settings["loose_wires"]:
        body = cv2.morphologyEx(solid.astype(np.uint8), cv2.MORPH_OPEN, disk(5))
        body = cv2.morphologyEx(body, cv2.MORPH_CLOSE, disk(30))
        filled = cv2.copyMakeBorder(body, 1, 1, 1, 1, cv2.BORDER_CONSTANT, value=0)
        cv2.floodFill(filled, None, (0, 0), 2)
        enclosed = (filled[1:-1, 1:-1] != 2)
        wire = solid | (gradient > 5.0)
        wire = cv2.dilate(wire.astype(np.uint8), disk(2)).astype(bool)
        # Grey rollers sit between the blue of a wheel: stay well clear of thick colour.
        colour = cv2.morphologyEx((chroma > 30.0).astype(np.uint8), cv2.MORPH_OPEN, disk(6))
        colour = cv2.dilate(colour, disk(45)).astype(bool)
        desk |= ~enclosed & ~wire & ~colour
    return desk & ~keep


def flatten_desk(lin_exposed, desk, settings):
    """Divides the desk's lighting away. Returns (picture, subject weight)."""
    h, w = desk.shape
    core = cv2.erode(desk.astype(np.uint8), disk(2)).astype(np.float32)
    level = smooth_fill(lin_exposed, core)
    level = np.maximum(level, 1e-3)

    # Distance from the object, for the contact shadow.
    distance = cv2.distanceTransform(desk.astype(np.uint8), cv2.DIST_L2, 5)
    reach = settings["shadow_reach"]
    fade = np.clip(1.0 - distance / reach, 0.0, 1.0)
    fade = fade * fade * (3 - 2 * fade)

    target = DESK_TARGET
    shadow = np.clip(luminance(level) / target, 0.05, 1.0)
    contact = shadow ** (settings["shadow_keep"] * fade)

    # Fine texture relative to the local level, compressed.
    ratio = np.clip(lin_exposed / level, 0.2, 1.6)
    ratio = np.where(ratio < 1.0, ratio ** settings["texture"], ratio ** settings["bright_texture"])
    desk_out = target * ratio * contact[..., None]

    # The edge between object and desk follows the picture's own edges (a guided filter), so
    # that it is neither a jagged mask line nor a halo.
    mask = (~desk).astype(np.float32)
    guide = linear_to_srgb(luminance(lin_exposed))
    subject = np.clip(guided_filter(guide, mask, 5, 4e-4), 0.0, 1.0)
    core = cv2.erode(mask, disk(1))
    near = cv2.dilate(mask, disk(5))
    subject = np.maximum(subject, core) * near
    return desk_out, cv2.GaussianBlur(subject, (0, 0), 0.6)


def guided_filter(guide, x, radius, eps):
    """Smooths x while following the edges of guide."""
    k = (2 * radius + 1, 2 * radius + 1)
    mean_g = cv2.blur(guide, k)
    mean_x = cv2.blur(x, k)
    var = cv2.blur(guide * guide, k) - mean_g * mean_g
    cov = cv2.blur(guide * x, k) - mean_g * mean_x
    a = cov / (var + eps)
    b = mean_x - a * mean_g
    return cv2.blur(a, k) * guide + cv2.blur(b, k)


def highlight_shoulder(lin, start=0.80):
    """Rolls highlights off smoothly, so that shiny parts do not clip."""
    over = np.maximum(lin - start, 0.0)
    room = 1.0 - start
    return np.minimum(lin, start) + room * (1.0 - np.exp(-over / room))


def grade_subject(lin, settings):
    """Tone curve, saturation, local contrast and sharpening. Input and output linear RGB."""
    srgb = linear_to_srgb(highlight_shoulder(lin))
    lab = cv2.cvtColor(srgb, cv2.COLOR_RGB2LAB)
    light = lab[..., 0] / 100.0

    # Local contrast, from an edge-preserving base so that there are no halos at edges.
    base = guided_filter(light, light, 18, 0.004)
    light = light + settings["clarity"] * (light - base)

    # Fine sharpening, with the overshoot limited.
    soft = cv2.GaussianBlur(light, (0, 0), 1.0)
    detail = np.clip(light - soft, -0.06, 0.06)
    light = light + settings["sharpen"] * detail

    # A gentle S curve around the middle. It leaves black and white where they are.
    light = np.clip(light, 0.0, 1.0)
    curve = light * light * (3 - 2 * light)
    light = light + settings["contrast"] * (curve - light)

    # Saturation: more on the coloured parts, none on the greys, so that greys stay neutral.
    a, b = lab[..., 1], lab[..., 2]
    chroma = np.hypot(a, b)
    gain = 1.0 + (settings["saturation"] - 1.0) * np.clip(chroma / 25.0, 0.0, 1.0)
    lab = np.dstack([light * 100.0, a * gain, b * gain])
    return srgb_to_linear(np.clip(cv2.cvtColor(lab, cv2.COLOR_LAB2RGB), 0.0, 1.0))


def process(lin, settings):
    """The whole calibration. Returns (linear RGB out, desk mask or None, report dict)."""
    white = measure_white(lin)
    closeup = settings["mode"] == "closeup"
    if white is None:
        white = np.array([0.8, 0.8, 0.8], np.float32)
        wb = np.ones(3, np.float32)
    else:
        wb = white.mean() / white
    if closeup:
        wb = np.clip(wb, 0.90, 1.12)
    lum_white = float(np.dot(white * wb, [0.2126, 0.7152, 0.0722]))
    gain = min(DESK_TARGET / max(lum_white, 1e-3) * settings["exposure"], settings["max_gain"])
    balanced = lin * wb[None, None, :] * gain
    report = dict(white=white * 255, wb=wb, gain=gain)

    if closeup:
        return grade_subject(balanced, settings), None, report

    desk = find_desk(np.clip(balanced, 0, 1.5), settings)
    desk_out, subject = flatten_desk(balanced, desk, settings)
    graded = grade_subject(balanced, settings)
    out = graded * subject[..., None] + desk_out * (1 - subject[..., None])
    return out, desk, report


def save_jpeg(lin, path):
    """Writes an sRGB JPEG with no metadata at all. Lowers the quality only if it is too big."""
    array = np.round(linear_to_srgb(lin) * 255.0).astype(np.uint8)
    image = Image.fromarray(array, "RGB")
    quality = JPEG_QUALITY
    while True:
        buffer = io.BytesIO()
        image.save(buffer, "JPEG", quality=quality, optimize=True, progressive=True,
                   subsampling="4:2:0")
        if buffer.tell() <= MAX_BYTES or quality <= JPEG_QUALITY_MIN:
            break
        quality -= 1
    with open(path, "wb") as f:
        f.write(buffer.getvalue())
    return buffer.tell(), quality


def run_one(job):
    number, debug_dir = job
    cv2.setNumThreads(1)
    settings = dict(DEFAULTS)
    settings.update(SETTINGS.get(number, {}))
    image = load_upright_srgb(os.path.join(IN_DIR, "IMG_%s.JPG" % number))
    lin, pad = frame(image, settings)
    out, desk, report = process(lin, settings)
    out = extend_desk(out, desk, pad)
    size, quality = save_jpeg(out, os.path.join(OUT_DIR, "IMG_%s.jpg" % number))
    if debug_dir and desk is not None:
        view = np.round(linear_to_srgb(lin) * 255).astype(np.uint8).copy()
        view[desk] = (view[desk] * 0.45 + np.array([255, 0, 255]) * 0.55).astype(np.uint8)
        Image.fromarray(view).save(os.path.join(debug_dir, "mask_%s.jpg" % number), quality=80)
    return (number, out.shape[1], out.shape[0], size, quality, report["gain"],
            report["white"].round().astype(int).tolist(), settings["mode"])


def main():
    parser = argparse.ArgumentParser(description="Make docs/images/photos from the originals.")
    parser.add_argument("numbers", nargs="*", help="photo numbers, for example 6934; default all")
    parser.add_argument("--debug", metavar="DIR", help="also write desk masks to this folder")
    args = parser.parse_args()

    names = []
    if os.path.isdir(IN_DIR):
        names = sorted(f for f in os.listdir(IN_DIR) if f.startswith("IMG_") and f.endswith(".JPG"))
    if not names:
        sys.exit("No originals found in docs/images/originals/.\n"
                 "The full-size photos are not in the repository, so this script runs only where\n"
                 "they have been placed by hand. The docs and tools/label_photos.py need only the\n"
                 "processed photos in docs/images/photos/, which are in the repository.")
    numbers = args.numbers or [f[4:-4] for f in names if f[4:-4] not in NOT_USED]
    missing = [n for n in numbers if "IMG_%s.JPG" % n not in names]
    if missing:
        sys.exit("No original for: " + ", ".join(missing))
    os.makedirs(OUT_DIR, exist_ok=True)
    if args.debug:
        os.makedirs(args.debug, exist_ok=True)

    with multiprocessing.Pool(min(8, os.cpu_count() or 1)) as pool:
        results = pool.map(run_one, [(n, args.debug) for n in numbers])
    for number, w, h, size, quality, gain, white, mode in results:
        print("IMG_%s  %4dx%-4d  %3d KB  q%d  gain %.2f  white %s  %s"
              % (number, w, h, size // 1024, quality, gain, white, mode))


if __name__ == "__main__":
    main()
