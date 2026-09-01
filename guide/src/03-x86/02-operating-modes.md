# x86 Operating Modes

For this guide, we will work with two x86 modes: 16-bit
real mode and 32-bit protected mode, but what exactly is an operating
mode? An operating mode refers to a specific configuration in which the 
CPU operates. Each mode defines how the CPU handles things such as memory
access, instructions, and privilege levels, and each offers different features,
capabilities, and limitations.

Protected and real mode are not the only modes in x86,
there is long mode (which provides 64-bit mode and compatibility mode), and 
many others. As we are writing a 32-bit
x86 operating system, our goal is to get into protected mode, which
provides the features we need to start building our OS.
When we started writing our OS in the last chapter, we
were working in real mode, as an x86 processor starts in real-address mode
after a reset.

Here is everything about real mode:

-   Real mode is a minimalist environment, unsurprisingly providing only
    the essential features required to bootstrap a computer system. It 
    lacks many of the advanced features that we will need in protected mode,
    such as memory protection.
-   Real mode also allows software to directly access memory and I/O
    without OS intervention, which allows for low-level manipulation of
    hardware components. It also allows us to use BIOS interrupts, which
    (like we used before) are commonly used during system boot-up and for
    low-level system programming tasks in real mode.
    BIOS interrupt services provide a way for software to request services
    from the system BIOS.
-   Now, onto limitations, the most important is probably a lack of
    memory protection (which, if you like Cybersecurity, you'd be
    interested in), real mode provides no memory protection
    mechanisms, so software running in real mode can access memory
    without the protection mechanisms provided by protected mode.
    Software running in real mode
    can generally access and modify without hardware-enforced protection, 
    leading to security and stability issues.

And now, let's talk about Protected Mode:

-   The advantages of protected mode are really just the disadvantages
    of real mode. It has memory protection, privilege levels, and 
    support for features needed to implement multitasking, among
    other things.
-   Protected mode provides added complexity compared to real mode
    because of the advanced features mentioned before.

That just about sums up what we need to know about our x86 operating
modes. As a summary, we are currently in real mode and need to get 
into protected mode to gain many useful features for making our
own OS.

