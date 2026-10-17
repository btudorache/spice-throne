#!/usr/bin/env python3
"""Build and run the desktop simulator.

    python tools/sim/run.py shots <keys> [out_dir]   screenshots -> PNG + contact sheet
    python tools/sim/run.py soak <minutes> [seed]     random input through the real game
    python tools/sim/run.py stats <runs> [seed]       a simple bot plays whole runs

Keys: L P R tap a lane, 1 2 3 hold a lane, s/w/W wait 1/5/60 s. Needs a C
compiler (gcc or clang; override with CC) and Pillow for PNG conversion.
"""
import glob
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
FW = os.path.join(ROOT, "spice_throne")
SRCS = ["run.c", "field.c", "perks.c", "title.c", "gfx.c", "text.c", "leds.c", "assets_gen.c"]
EXE = os.path.join(HERE, "sim.exe" if os.name == "nt" else "sim")


def build():
    cc = os.environ.get("CC") or shutil.which("gcc") or shutil.which("clang") or "cc"
    cmd = [cc, "-O2", "-std=gnu99", "-Wall", "-Wextra", "-Wno-unused-parameter", "-o", EXE,
           os.path.join(HERE, "sim.c")] + [os.path.join(FW, s) for s in SRCS]
    subprocess.run(cmd, check=True)


def shots(keys, out):
    from PIL import Image
    os.makedirs(out, exist_ok=True)
    for f in glob.glob(os.path.join(out, "shot_*")):
        os.remove(f)
    subprocess.run([EXE, "shots", out, keys], check=True)
    frames = []
    for ppm in sorted(glob.glob(os.path.join(out, "shot_*.ppm"))):
        im = Image.open(ppm).convert("RGB")
        im.save(ppm[:-4] + ".png")
        os.remove(ppm)
        frames.append(im)
    if frames:
        cols = min(6, len(frames))
        rows = (len(frames) + cols - 1) // cols
        w, h = frames[0].size
        sheet = Image.new("RGB", (cols * (w + 4), rows * (h + 4)), (40, 40, 40))
        for i, im in enumerate(frames):
            sheet.paste(im, ((i % cols) * (w + 4), (i // cols) * (h + 4)))
        sheet = sheet.resize((sheet.width * 2, sheet.height * 2), Image.NEAREST)
        sheet.save(os.path.join(out, "sheet.png"))
    print(f"{len(frames)} screenshots in {out}")


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    build()
    if sys.argv[1] == "shots":
        shots(sys.argv[2], sys.argv[3] if len(sys.argv) > 3 else os.path.join(HERE, "out"))
    else:
        subprocess.run([EXE] + sys.argv[1:], check=True)


if __name__ == "__main__":
    main()
