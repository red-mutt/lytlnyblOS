## x86 Operating Modes

We work with two modes in x86; these are 16-bit
real mode and 32-bit protected mode, but what exactly is an operating
mode? An operating mode refers to a specific configuration in which the
CPU operates, each mode defines how the CPU interacts with memory,
hardware and software, they each offer many different features,
capabilities, and limitations.

Protected and real mode are not the only modes in x86,
there is long mode (which only exists in 64-bit systems), compatibility
mode (which is 16-bit) and many others. But as we are writing a 32-bit
x86 operating system, our only goal is to get into protected mode, which
is mainly used for modern operating systems and software, which is what
we are making. When we started writing our OS in the last chapter, we
were working in real mode, as real mode is the initial operating mode of
x86 processors during system boot-up.

Here is everything about real mode:

-   Real mode is a minimalist environment, unsurprisingly providing only
    the essential features required to bootstrap a computer system. It
    lacks many advanced features that we will need to access in
    protected mode, Such as memory protection
-   Real mode also has direct access to resources without abstraction
    layers or OS intervention. Which allows for low level manipulation
    of hardware components. It also allows access to BIOS interrupts,
    which (like we used before) are commonly used during system boot-up
    and for low-level system programming tasks performed in real mode.
    BIOS interrupts provide a way for software to interact with the
    system BIOS.
-   Now, onto limitations, the most important is probably a lack of
    memory protection (which, if you like Cybersecurity, you'd be
    interested in), real mode offers absolutely no memory protection
    mechanisms, leaving the system utterly vulnerable to memory
    corruption and unauthorized access. Software running in real mode
    can freely access and change any memory location, leading to
    security and stability issues.

And now, let's talk about Protected Mode:

-   The advantages of protected mode are really just the disadvantages
    of Real mode. It has Memory Protection, Multitasking support, and
    privilege levels, among other things.
-   Protected mode provides added complexity compared to
    real mode due to its advanced features mentioned before.

That just about sums up what we need to know about our x86 operating
modes. As a summary, we are currently in real mode and need to get into
our protected mode to get many useful features for making our own OS.

