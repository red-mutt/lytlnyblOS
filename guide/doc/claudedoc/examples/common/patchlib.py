"""
Tiny helper for the examples that insert code into existing files
rather than replacing them whole.

Every edit is anchored on an exact piece of existing text and fails
loudly if that text is missing or ambiguous, so an example can never
silently "apply" and then not work.

Usage from an example's patch.py:

    import sys, os
    sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "common"))
    from patchlib import Patcher

    p = Patcher(sys.argv[1])            # the build/os tree
    p.after("include/kernel/interrupts.h",
            "#define SYSCALL_CLOSE 0x0A",
            "#define SYSCALL_UPTIME 0x0B")
    p.done()
"""

import os
import sys


class PatchError(Exception):
    pass


class Patcher:
    def __init__(self, root):
        self.root = root
        self.count = 0

    def _read(self, rel):
        path = os.path.join(self.root, rel)
        if not os.path.exists(path):
            raise PatchError("file not found: %s" % rel)
        with open(path, encoding="utf-8") as fh:
            return path, fh.read()

    def _check(self, rel, text, anchor):
        n = text.count(anchor)
        if n == 0:
            raise PatchError(
                "anchor not found in %s:\n    %s\n"
                "The original file has probably changed since this example "
                "was written." % (rel, anchor.strip().splitlines()[0]))
        if n > 1:
            raise PatchError(
                "anchor appears %d times in %s, so the insertion point is "
                "ambiguous:\n    %s" % (n, rel, anchor.strip().splitlines()[0]))

    def _write(self, path, text):
        with open(path, "w", encoding="utf-8") as fh:
            fh.write(text)
        self.count += 1

    def after(self, rel, anchor, addition):
        """Insert `addition` on the line after `anchor`."""
        path, text = self._read(rel)
        self._check(rel, text, anchor)
        self._write(path, text.replace(anchor, anchor + "\n" + addition, 1))
        print("    + %s (after %r)" % (rel, _short(anchor)))

    def before(self, rel, anchor, addition):
        """Insert `addition` on the line before `anchor`."""
        path, text = self._read(rel)
        self._check(rel, text, anchor)
        self._write(path, text.replace(anchor, addition + "\n" + anchor, 1))
        print("    + %s (before %r)" % (rel, _short(anchor)))

    def replace(self, rel, old, new):
        """Replace an exact, unique piece of text."""
        path, text = self._read(rel)
        self._check(rel, text, old)
        self._write(path, text.replace(old, new, 1))
        print("    ~ %s (%r)" % (rel, _short(old)))

    def append(self, rel, addition):
        """Add text at the end of a file."""
        path, text = self._read(rel)
        self._write(path, text.rstrip("\n") + "\n" + addition + "\n")
        print("    + %s (appended)" % rel)

    def done(self):
        print("    %d edit(s) applied" % self.count)


def _short(s):
    line = s.strip().splitlines()[0]
    return line if len(line) <= 46 else line[:43] + "..."


def run(fn):
    """Wrap an example's patch function with clean error reporting."""
    if len(sys.argv) < 2:
        print("usage: patch.py <path to build/os tree>", file=sys.stderr)
        return 2
    try:
        fn(Patcher(sys.argv[1]))
    except PatchError as exc:
        print("PATCH FAILED: %s" % exc, file=sys.stderr)
        return 1
    return 0
