# lytlnyblOS

A small, from scratch 32-bit operating system, built to teach beginners practically
about OS development. Built alongside [The lytlnyblOS Book](https://red-mutt.github.io/lytlnyblOS/), 
a guide that walks through writing an OS from a 16-bit bootloader all the way through protected mode,
interrupts, memory management, processes, a filesystem, userspace, and a working shell.

The code in this repo is the code taught in the book. Every part of it corresponds to something
explained in the book, this is the whole point of the project.

## Read the book

**[📖 red-mutt.github.io/lytlnyblOS](https://red-mutt.github.io/lytlnyblOS/)**

The book goes in depth explaining *why* something in the OS works the way it does, not 
just what it does.

## What the directories in the repository are for

-  `guide/` - Source for the book (made with mdbook).
-  `lytlnybl/` - Source for the operating system.
-  `blog_playground/` - Shows a history of the OS's creation, not intended for use and is unmaintained.

## Building and running
You will need:
-  NASM
-  An `i686` cross-compiler for C
-  QEMU
-  Make
-  GDB

`make run` in `lytlnybl/` assembles the bootloader and kernel, links them, writes the result
into the disk image, and boots it in QEMU.

## Status

The guide and its accompanying source are complete, start to finish.

## Found an error in the guide?

**Please [open an issue](../../issues)**. This is by far the most useful
way to help this project. 

If something in the book is unclear, technically wrong, out of date, or just 
doesn't match the code in `lytlnybl/`, an issue takes two minutes to make
and helps everyone who reads the guide after you. You don't need to know what to
fix, just describe what you found:

-  A typo or confusing explanation
-  A technical inaccuracy
-  Code in the repo that no longer matches what the book describes
-  A question about something that confuses you

## Contributing

Contributions are welcome, but scope is intentionally limited, please read 
[CONTRIBUTING.md](CONTRIBUTING.md) before opening a pull request.

## License

Both the guide and source code have two different licenses, both found in the respective `guide/` and 
`lytlnybl/` directories.

## Acknowledgements
 
See the book's [Introduction](https://red-mutt.github.io/lytlnyblOS/introduction.html) for acknowledgements.

