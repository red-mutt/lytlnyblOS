# lytlnyblOS Learning Manual

`lytlnyblOS-Learning-Manual.pdf` is an original, beginner-oriented companion to the final source tree. It explains the system in the order in which it starts and grows: firmware, boot sector, protected mode, kernel services, memory, tasks, user programs, and the filesystem. Its central virtual-memory chapter starts with an intuitive model and then walks through the exact paging code, addresses, flags, TLB behaviour, page faults, and user address spaces used by this OS.

The second half is a guided reference to the complete build-relevant source tree. Each file is grouped by subsystem and introduced with a study objective, so the manual can be used both as a tutorial and as a line-by-line companion while working in the code.

The PDF is generated from `lytlnyblOS-Learning-Manual.tex`. On a Linux system with a LaTeX installation, rebuild it with:

```sh
cd doc
pdflatex lytlnyblOS-Learning-Manual.tex
pdflatex lytlnyblOS-Learning-Manual.tex
```

The second pass builds the table of contents.
