# Runnable examples

One folder per chapter. Each contains a small, self-contained program that
exercises what that chapter explains, plus a Makefile that builds a complete
bootable image and launches it in QEMU.

```sh
cd ch27-paging
make run          # build and boot it in a QEMU window
make shell        # boot headless and print the text screen (works over ssh)
make diff         # show exactly what this example changes vs. the original
make clean        # delete this example's build directory
make              # print the available targets
```

**The project in `lytlnybl/` is never modified.** Each example copies the OS
tree into its own `build/os/`, overlays its files on the copy, builds that, and
boots it. Delete `build/` and nothing is left behind.

## The examples

| Folder | Chapter | What it demonstrates |
|--------|---------|----------------------|
| `ch12-bootloader` | 12 | Editing the 512-byte boot sector, printing via the BIOS |
| `ch16-gdt` | 16 | Reading the live GDT with `SGDT`; the Accessed and Busy bits |
| `ch18-vga-output` | 18 | Every text-driver operation, and the `write_dec(0)` bug |
| `ch21-custom-interrupt` | 21 | Adding your own IDT vector, in all three layers |
| `ch23-timer` | 23 | Timing work, and what changing the PIT frequency really changes |
| `ch24-keyboard-input` | 24 | Reading a line correctly, unlike the shell |
| `ch25-physical-memory` | 25 | `alloc_frame` / `free_frame`, and first-fit reuse |
| `ch27-paging` | 27 | Map, alias, translate, unmap, and build an address space |
| `ch28-page-fault` | 28 | Touch an unmapped address and read the report |
| `ch29-heap` | 29 | `kmalloc`/`kfree` with the block list printed each step |
| `ch30-kernel-process` | 30 | Three kernel processes preempted by the timer |
| `ch33-loader` | 33 | Loading a program from the kernel, with no shell |
| `ch34-new-syscall` | 34 | Adding a system call: the four files you must touch |
| `ch35-libc` | 35 | A user-program template using the whole libc |
| `ch37-ata-sectors` | 37 | Raw sector read and write, and finding the superblock |
| `ch39-filesystem` | 39 | Create, write, reopen, read, EOF, list, delete |
| `ch40-shell-command` | 40 | Adding `echo` and `rev` to the shell |
| `ch45-reboot-loop` | 45 | Reproducing the reboot loop, and tracing it |

## Two ways an example changes the system

Most examples set `OVERLAY` and drop in a replacement file — usually
`kernel_main.c`, since that is where the kernel's own demonstration code lives:

```make
OVERLAY = kernel_main.c:kernel/kernel_main.c
```

Examples that add to existing files instead use `PATCHES`, pointing at a
`patch.py` that makes anchored insertions:

```make
PATCHES = patch.py
```

Those patches fail loudly if an anchor is missing or ambiguous, so an example
can never appear to apply and then silently not work. Read `ch34-new-syscall/patch.py`
as the recipe for adding a system call: the four edits in it are the four
things you have to do.

## Checking they all still build

```sh
./verify.sh
```

Builds all eighteen from scratch and reports. Worth running after any change
to the OS the examples overlay — an overlay can go stale if the file it
replaces gains something the example does not know about.

## Requirements

The same toolchain the OS needs: `i686-elf-gcc`, `nasm`, `ld`, `make` and
`qemu-system-i386`.

`make shell` additionally needs Python 3 and a vgabios ROM to read the VGA font
from (`/usr/share/seabios/vgabios.bin` on Debian and Ubuntu, shipped with
QEMU). If it cannot find one it says so and you can use `make run` instead.
