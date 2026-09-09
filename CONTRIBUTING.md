# Contributing to lytlnyblOS

Thanks you for taking an interest in contributing to this project. There
are quite a few rules pertaining to contributions as this is a guide for making an operating system.

## What this project is

lytlnyblOS is a **guided** operating system, the code in this repo exists to be
explained, step by step, in [the accompanying book](https://red-mutt.github.io/lytlnyblOS/).
This relationship changes the way how contributions work:

-  Any change to the code should keep the book accurate. A change that alters behaviour
   without matching an update to the relevant chapter will likely need an update before it
   can be merged.
-  The scope is intentionally narrow.This project isn't meant to grow into a general-purpose OS.
   It is meant to stay a complete, coherent teaching example. New subsystems and major features are
   unlikely to be accepted unless they're something the guide itself is going to cover.

Because of these two points, most contributions are more useful as issues than as PRs.

## The best way to contribute: open an issue

Please [open an issue](../../issues) is you find:
-  A typo or confusing explanation
-  A bug in the OS's code
-  A technical inaccuracy
-  Code in the repo that no longer matches what the book describes
-  Something that confused you. If confuses you, it may confuse someone else. Please report this.

## Pull requests

Small, self-contained fixes are welcome directly:
-  Typo or grammar fixes in the book
-  Fixes for a bug in existing code (please explain the bug in PR description)
-  Small clarifications to existing explanations

For anything larger such as a new feature, a refactor, or a different implementation approach:
please open an issue first to discuss it before writing code. The code and the guide have to move together,
and a design has to fit with what the guide has already explained up to that point, so it's easy to do a lot of work
that can't be merged as is.

## Code style

The existing C and ASM aren't perfectly idiomatic in places. The plan is to clean it up over
time rather than all at once. Please keep PRs focused on a specific bug or issue you're addressing rather
than including a broader stylistic refactor.

## Tone

The book is written in one voice, as a single continuous narrative. If you're contributing prose, try to match the existing style rather
than introducing a noticeably different one.
