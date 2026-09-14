# Operating Systems from Scratch

A complete, from-zero course built around the lytlnyblOS source tree.

`course.pdf` is the output: 218 pages, thirteen parts plus appendices, written for a reader who
knows how to write a C program that prints "Hello" and nothing else about computer architecture,
assembly language or operating systems. It ends with that reader understanding every subsystem in
`lytlnybl/`, able to read a processor trace, and holding a list of concrete improvements they could
make to the code.

## What is in it

| Part | Chapters | Covers |
|------|----------|--------|
| I    | 1–5   | What a CPU, memory, the stack and I/O ports actually are |
| II   | 6–8   | What an operating system is; the seven minimum pieces; a map of the project |
| III  | 9–12  | Firmware, the boot sector, real mode, INT 13h, the E820 memory map |
| IV   | 13–16 | Protected mode, the GDT, the far jump, linker scripts, entering C |
| V    | 17–18 | VGA text mode and the text driver |
| VI   | 19–22 | Interrupts, the IDT, ISR stubs, the PIC and EOI |
| VII  | 23–24 | The PIT timer and the PS/2 keyboard |
| VIII | 25–29 | Physical memory, **virtual memory explained slowly**, paging in practice, page faults, the kernel heap |
| IX   | 30–31 | Processes, context switching, the scheduler |
| X    | 32–35 | The TSS, entering ring 3, system calls, a minimal libc |
| XI   | 36–39 | ATA PIO, filesystem design, descriptors, `mkfs` |
| XII  | 40    | The shell |
| XIII | 41–45 | The build system, kernel debugging, **a full worked debugging case study**, known issues, next steps |
| App. | A–F   | Glossary, x86 reference, memory maps, syscall reference, file index, exercise notes |

Chapter 26 (virtual memory) is the longest and slowest in the book, at 13 pages. It builds the
idea from nothing, discards three wrong solutions first, translates one address by hand digit by
digit and then a second one that *fails*, explains why a page is 4096 bytes rather than asserting
it, and finishes with a **complete address space dumped out of the running system** — the shell's
real page directory, its four page tables, and what every flag bit in them says about what the
program has actually done.

Everywhere else the length is even: median 4 pages, most chapters between 3 and 6. Part VIII
(memory) is the largest part at 30 pages, tied with the appendices, followed by Part XIII
(engineering) at 19 and Parts I and X at 17 each.

## Runnable examples

Eighteen of the chapters ship a working program you can build and boot, one folder each under
`examples/`. They are pointed at from teal "In practice" boxes in the text and indexed in
Appendix G.

```sh
cd examples/ch27-paging
make run        # build and boot it in QEMU
make shell      # boot headless and print the text screen (works over ssh)
make diff       # show exactly what this example changes
```

Each example copies the OS tree into its own `build/os/`, overlays its files on the copy and builds
that, so `lytlnybl/` is never touched. `examples/verify.sh` rebuilds all eighteen from scratch and
reports — worth running after any change to the OS they overlay.

Three of the findings in Chapter 46 came from writing and running these rather than from reading
the code, including the fact that the processor writes back into your own GDT.

## Building

Needs `pdflatex` with `listings`, `xcolor`, `tikz`, `mdframed`, `titlesec`, `enumitem`, `booktabs`
and `microtype`.

```sh
make            # two passes, produces curso.pdf
make clean      # remove auxiliary files
```

On a minimal TeX Live install, the missing packages can be added per-user without root:

```sh
mkdir -p ~/texmf && cd /tmp
B=https://mirror.ctan.org/systems/texlive/tlnet/archive
for p in listings xcolor pgf titlesec enumitem booktabs fancyvrb \
         needspace parskip etoolbox mdframed zref microtype caption; do
  curl -sfL -o $p.tar.xz $B/$p.tar.xz && tar -xf $p.tar.xz
done
cp -r tex ~/texmf/ && mktexlsr ~/texmf
```

## Source layout

| File | Contents |
|------|----------|
| `course.tex` | Master document: cover, front matter, part includes |
| `style.tex` | Preamble: palette, headings, code styles, the six box environments |
| `00-preface.tex` | How to use the course |
| `01-foundations.tex` … `13-engineering.tex` | One file per part |
| `A-appendices.tex` | All appendices |
| `examples/` | The eighteen runnable examples |

The six box environments are `concept` (blue, a central idea), `analogy` (purple, intuition),
`warn` (amber, a trap), `danger` (red, something that breaks the system), `tryit` (green, something
you can run to verify the text) and `recipe` (teal, a pointer to a runnable example under
`examples/`).

## A note on accuracy

Everything asserted about the code was checked against the source, and the runtime behaviour quoted
in the text — boot output, shell sessions, the processor trace in Chapter 45 — was captured from
the system actually running under QEMU, not reconstructed. The bugs listed in Chapter 46 are real
and were found while writing this; several are reproducible on the current tree.
