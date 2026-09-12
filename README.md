# lytlnyblOS

lytlnyblOS (little nibble OS) is a small, from scratch 32-bit operating system, built to teach beginners practically
about OS development. Ran on a QEMU virtual machine. Built alongside [The lytlnyblOS Book](https://red-mutt.github.io/lytlnyblOS/), 
a guide that walks through writing an OS from a 16-bit bootloader all the way through protected mode,
interrupts, memory management, processes, a filesystem, userspace, and a working shell.

The code in this repo is the code taught in the book. Every part of it corresponds to something
explained in the book, this is the whole point of the project.

Here's a little video demo of what you'll be making:

https://github.com/user-attachments/assets/f02ed3cf-41a2-4a80-85b7-0d6aa5f0f7ae

This may not look like much, but remember it's your own custom operating system that you'll have freedom to do whatever you want with by the end of the book! Every line of code will be written by you!

## Read the book

**[📖 red-mutt.github.io/lytlnyblOS](https://red-mutt.github.io/lytlnyblOS/)**

The book goes in depth explaining *why* something in the OS works the way it does, not 
just what it does. You'll make:

- A bootloader
- An entry into x86 Protected mode
- GDTs & IDTs
- VGA text mode
- Interrupts
- A timer driver
- A keyboard driver
- A memory manager
- Paging systems
- Processes
- Scheduling
- User mode
- System Calls
- A C standard libary
- Filesystems
- A MKFS
- And finally, a shell

All in this order

## Project status

The project is **complete, but with ongoing refinement.**

The operating system and accompanying guide are complete from start to finish. The project currently covers the full journey from bootloader to working
user-space shell and has not been abandoned. I consider this project *feature complete for its educational goals*. Future work will be mainly
focused around refining the guide, improving explanations, fixing mistakes, and making the experience easier for others to follow.

Feedback and corrections are welcome, particularly from people working through the guide themselves. Feel free to read [CONTRIBUTING.md](CONTRIBUTING.md)

## What the directories in the repository are for

-  `guide/` - Source for the book (made with mdbook).
-  `lytlnybl/` - Source for the operating system.
-  `blog_playground/` - Shows a history of the OS's creation, not intended for use and is unmaintained.

## Building and running
No special hardware is required for this project. You only will need:
-  NASM
-  An `i686` cross-compiler for C
-  QEMU
-  Make
-  GDB

`make run` in `lytlnybl/` assembles the bootloader and kernel, links them, writes the result
into the disk image, and boots it in QEMU.

This book assumes that you have access to many unix-like command, like `dd`, `rm`, `mkdir`, and `cp`. Linux is the primary supported environment.
**Windows users can use WSL.**

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

