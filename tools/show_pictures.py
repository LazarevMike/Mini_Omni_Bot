"""Hides the pictures in the docs whose file does not exist yet, and shows them once it does.

GitHub shows a broken-image icon for every picture with a missing file. The picture lines
are written first and the photos taken later, so this script turns each missing picture
into a comment, and back into a picture when its file appears:

    ![The motor in M1](images/wiring/motor_in_m1.jpg)              file there: shown
    <!-- ![The motor in M1](images/wiring/motor_in_m1.jpg) -->     no file yet: hidden

The <!-- PHOTO: ... --> notes under each picture stay as they are.

After taking a photo, place it where docs/contributing/PICTURES.md states, then run:

    python3 tools/show_pictures.py

`python3 tools/show_pictures.py --check` changes nothing, and exits with 1 if a page needs
fixing. It reads every .md file in the project, except in hidden folders (.git, .pio).

Note: a hidden picture returns as soon as its file exists. To remove a picture permanently,
delete its line instead of commenting it out.
"""

import argparse
import os
import re
import sys
from pathlib import Path
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parent.parent  # the project folder, one up from tools/

# A picture on a line of its own, and the same line hidden. The indent is kept, so a
# picture inside a numbered step stays inside its step.
SHOWN = re.compile(r"^(\s*)(!\[[^\]]*\]\(([^)\s]+)\))\s*$")
HIDDEN = re.compile(r"^(\s*)<!--\s*(!\[[^\]]*\]\(([^)\s]+)\))\s*-->\s*$")


def pages():
    """Every .md file in the project, skipping hidden folders."""
    for folder, subfolders, files in os.walk(ROOT):
        subfolders[:] = sorted(d for d in subfolders if not d.startswith("."))
        for name in sorted(files):
            if name.endswith(".md"):
                yield Path(folder) / name


def file_is_there(page, link):
    """True if the picture's file exists, spelt exactly the way the link spells it."""
    path = page.parent / unquote(link)
    # Windows and macOS ignore upper and lower case, but GitHub does not: a photo saved as
    # hero.JPG does not show for a link to hero.jpg. The exact name is therefore checked too.
    return path.is_file() and path.name in os.listdir(path.parent)


def fix_page(page, text, changes):
    """Hides or shows each picture line on one page, and returns the new text."""
    name = page.relative_to(ROOT).as_posix()
    lines = text.split("\n")
    in_code = False
    for n, line in enumerate(lines):
        body = line.rstrip("\r")
        ending = line[len(body):]  # keep Windows line endings if the page has them
        if body.lstrip().startswith(("```", "~~~")):
            in_code = not in_code  # examples inside code blocks are not pictures
            continue
        shown = None if in_code else SHOWN.match(body)
        hidden = None if in_code else HIDDEN.match(body)
        if not (shown or hidden):
            continue
        indent, picture, link = (shown or hidden).groups()
        if link.startswith(("http:", "https:", "data:", "/", "#")):
            continue  # a web address, not a file in the project
        there = file_is_there(page, link)
        if shown and not there:
            lines[n] = f"{indent}<!-- {picture} -->{ending}"
            changes.append(("hide", name, n + 1, link))
        elif hidden and there:
            lines[n] = f"{indent}{picture}{ending}"
            changes.append(("show", name, n + 1, link))
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(
        description="Hide the docs' pictures whose file is missing, show the rest.")
    parser.add_argument("--check", action="store_true",
                        help="change nothing, exit with 1 if a page needs fixing")
    check = parser.parse_args().check

    changes = []
    for page in pages():
        # newline="" reads and writes the page's line endings exactly as they are
        with open(page, encoding="utf-8", newline="") as f:
            text = f.read()
        new_text = fix_page(page, text, changes)
        if new_text != text and not check:
            with open(page, "w", encoding="utf-8", newline="") as f:
                f.write(new_text)

    would = "would " if check else ""
    for what, name, line, link in changes:
        note = "  (no file yet)" if what == "hide" else ""
        print(f"{would}{what}  {name}:{line}  {link}{note}")
    hid = sum(1 for change in changes if change[0] == "hide")
    showed = len(changes) - hid
    if not changes:
        print("All good: every picture with a file shows, every one without is hidden.")
    elif check:
        print(f"{hid} to hide, {showed} to show. Run python3 tools/show_pictures.py to fix.")
        sys.exit(1)
    else:
        print(f"Done: {hid} hidden, {showed} shown.")


if __name__ == "__main__":
    main()
