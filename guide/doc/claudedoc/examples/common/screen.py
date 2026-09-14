#!/usr/bin/env python3
"""
Boot a lytlnyblOS disk image in QEMU with no display, optionally type
something at the shell, and print what is on the text screen.

    python3 screen.py kernel.img                 boot and print the screen
    python3 screen.py kernel.img --wait 12       wait longer before capturing
    python3 screen.py kernel.img --type "ls /"   type a line, then capture

Useful for checking an example works over ssh, in CI, or anywhere without
a graphical session.

How it works: QEMU's monitor can save a PNG/PPM of the framebuffer
(`screendump`). The screen is 720x400, which is 80x25 cells of 9x16
pixels, and the glyphs come from the standard VGA 8x16 font that ships
inside the vgabios ROM. So each cell can be matched back to a character
and the screen recovered as text.
"""

import argparse
import glob
import os
import re
import socket
import subprocess
import sys
import tempfile
import time
from collections import Counter

# The 'A' glyph of the standard IBM VGA 8x16 font, used to locate the
# font table inside a vgabios ROM without hardcoding an offset.
GLYPH_A = bytes([0x00, 0x00, 0x10, 0x38, 0x6C, 0xC6, 0xC6, 0xFE,
                 0xC6, 0xC6, 0xC6, 0xC6, 0x00, 0x00, 0x00, 0x00])

ROM_PATHS = [
    "/usr/share/seabios/vgabios.bin",
    "/usr/share/qemu/vgabios.bin",
    "/usr/share/seabios/vgabios-stdvga.bin",
    "/usr/share/qemu/vgabios-stdvga.bin",
]


def load_font():
    """Return {16-byte bitmap: character code} from a vgabios ROM."""
    for path in ROM_PATHS + sorted(glob.glob("/usr/share/*/vgabios*.bin")):
        if not os.path.exists(path):
            continue
        data = open(path, "rb").read()
        i = data.find(GLYPH_A)
        if i < 0:
            continue
        base = i - 65 * 16          # 'A' is character 65
        if base < 0:
            continue
        font = data[base:base + 4096]
        return {bytes(font[c * 16:c * 16 + 16]): c for c in range(256)}
    return None


def decode(ppm, glyphs):
    """Turn a 720x400 screendump into 25 lines of text."""
    raw = open(ppm, "rb").read()
    parts = raw.split(b"\n", 3)
    if len(parts) < 4 or not parts[0].startswith(b"P6"):
        return "(could not parse screendump)"
    width, height = map(int, parts[1].split())
    px = parts[3]

    def colour(x, y):
        i = (y * width + x) * 3
        return px[i], px[i + 1], px[i + 2]

    lines = []
    for row in range(height // 16):
        out = ""
        for col in range(width // 9):
            cell = [[colour(col * 9 + k, row * 16 + y) for k in range(8)]
                    for y in range(16)]
            # The most common colour in the cell is the background.
            background = Counter(p for r in cell for p in r).most_common(1)[0][0]
            bitmap = bytes(
                sum((1 << (7 - k)) if cell[y][k] != background else 0
                    for k in range(8))
                for y in range(16))
            ch = glyphs.get(bitmap)
            if ch is not None and 32 <= ch < 127:
                out += chr(ch)
            else:
                out += " "
        lines.append(out.rstrip())
    while lines and not lines[-1]:
        lines.pop()
    return "\n".join(lines)


KEYMAP = {" ": "spc", "/": "slash", ".": "dot", "-": "minus",
          "_": "shift-minus", "\n": "ret", ",": "comma"}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("image")
    ap.add_argument("--wait", type=float, default=9.0,
                    help="seconds to let the system boot (default 9)")
    ap.add_argument("--type", default=None,
                    help="text to type once booted; \\n sends Enter")
    ap.add_argument("--settle", type=float, default=2.0,
                    help="seconds to wait after typing (default 2)")
    ap.add_argument("--linepause", type=float, default=1.5,
                    help="seconds to wait after each Enter (default 1.5)")
    args = ap.parse_args()

    glyphs = load_font()
    if glyphs is None:
        print("Could not find a vgabios ROM to read the VGA font from.",
              file=sys.stderr)
        print("Run 'make run' instead to see the screen directly.",
              file=sys.stderr)
        return 2

    # A short socket path: AF_UNIX paths are limited to ~108 bytes.
    sockdir = tempfile.mkdtemp(prefix="lyt")
    sock_path = os.path.join(sockdir, "m")
    shot = os.path.join(sockdir, "shot.ppm")

    qemu = subprocess.Popen(
        ["qemu-system-i386", "-drive", "format=raw,file=" + args.image,
         "-display", "none", "-monitor", "unix:%s,server,nowait" % sock_path],
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    try:
        for _ in range(80):
            if os.path.exists(sock_path):
                break
            time.sleep(0.1)
        mon = socket.socket(socket.AF_UNIX)
        mon.connect(sock_path)

        def cmd(text, pause=0.2):
            mon.sendall((text + "\n").encode())
            time.sleep(pause)

        time.sleep(args.wait)

        if args.type:
            for ch in args.type.replace("\\n", "\n"):
                # The keyboard driver echoes every key the moment it
                # arrives, so after Enter we must let the shell finish
                # running the command and print its output before
                # sending the next character -- otherwise the echo of
                # the next key lands in the middle of that output.
                cmd("sendkey " + KEYMAP.get(ch, ch),
                    args.linepause if ch == "\n" else 0.10)
            time.sleep(args.settle)

        cmd('screendump "%s"' % shot, 1.0)
        for _ in range(60):
            if os.path.exists(shot) and os.path.getsize(shot) > 100000:
                break
            time.sleep(0.1)
        time.sleep(0.3)

        print("-" * 80)
        print(decode(shot, glyphs))
        print("-" * 80)
    finally:
        qemu.kill()
        for f in (shot, sock_path):
            if os.path.exists(f):
                os.remove(f)
        os.rmdir(sockdir)
    return 0


if __name__ == "__main__":
    sys.exit(main())
