## x86 Interrupts

If you've ever made a website in Javascript and made something happen
when you clicked a button, you'd know about event-driven programming
(or at least would have used it before). Event-driven programming is a
programming paradigm in which the flow of a program is determined by
external events. This paradigm will also be used when developing our
operating system, and it comes in the form of interrupts.

An interrupt is simply a type of instruction that is used when an event
occurs and halts whatever process the processor is processing. The
processor then calls the required interrupt service routine that
respects the current interrupt; after doing this, the processor returns
to the process it was handling before.

There are many kinds of interrupts, and we've actually used one before;
this was when we were loading the kernel into memory from the disk; this
was a disk interrupt; there are video interrupts, etc. An interrupt
happens when you hit a key on a keyboard; these will then be handled in
a certain way by the operating system that consults correctly with the
device drivers.

Either hardware or software can interrupt the processor. The keyboard is
an example of a hardware interrupt, but ones can be caused by software

Interrupts all have an interrupt number, which specifies what interrupt
it is; the one we used in our bootloader was 10h, when we used [int
10h]{.hljs} It called the interrupt service routine numbered 10h. A
kernel can also give its own interrupts to be used by application
software; this can be stuff like manipulating file systems. and would be
used in the same way we used our BIOS interrupts; ones provided by the
kernel are called "system calls."

As well as interrupts, exceptions can also be used as another type of
event that would stop the processor in a similar way to how an interrupt
would. The difference however is that exceptions happen when a problem
occurs (an error).

### The interrupt descriptor table

In x86, there is another table called the "interrupt descriptor table"
(IDT). The IDT tells the processor how to reach the service descriptor
of a specific interrupt number. Entries in the IDT are called "gate
descriptors." The size of each gate descriptor is 8 bytes, the same as
descriptors in the GDT. The base address of the IDT is stored in a
register called the IDTR (Interrupt descriptor table register).

Gate descriptors in the IDT can be 1 of three types. The task gate, the
interrupt gate and trap gate. Focusing on the interrupt and trap gate, a
diagram of them can be seen here:

![Trap and Interrupt descriptor](images/os/trap%20and%20interrupt%20descriptor.png)

A gate descriptor should point to the memory address of the interrupt
service descriptor's code. You can see bytes 2 and 3 in both contain a
segment selector, which is the selector of the handler's code. The
offset of the service descriptor's is useful if the first instruction
of the service descriptor is a part of the code segment given by the
segment selector; as we can see, this is divided into parts like
descriptors in the GDT are.

The least significant nibble of byte 4 is reserved, and the most
significant nibble of byte 4 should be kept as 0. When the present flag
(P flag) is 0, this means that the code that the descriptor is pointing
to is not laoded into memory, and 1 means that it is. The DPL is the
privilege level of the service routine.

The D flag specifies the size of the gate descriptor itself. When D = 1,
the size is 32 bits, and 0 means 16 bits. 32 bits should always be used
in protected mode. There is also the T flag. which is the right most
next to the D flag; when this is 0, it is an interrupt gate; when it is
1, it is a trap gate, which can be seen in the respective diagrams.

The difference between interrupt and trap gates is that when an
interrupt gate is called, the processor is going to disable the ability
to signal a new interrupt until the service routine returns. There are
exceptions though; one of these is an interrupt known as "non-maskable
interrupts" (NMI), which will interrupt execution of an interrupt even
if it is caused by an interrupt gate. NMI's occur when something really
bad happens in the system.

service routines defined by a trap gate can be interrupted, where as
interrupt gates can't

Disabling interruption can also be performed by code using the
[cli]{.hljs} (clear interrupt flag) instruction. The ability to
interrupt the code can be enabled again by using the [sti]{.hljs} (set
interrupt flag) instruction, both of these manipulate the interrupt
flag, a part of EFLAGS.

It is also worth knowing, that the interrupt number, is just the index
of the interrupt service descriptor itself, and in protected mode, these
interrupt numbers have a specific meaning from 0-21. Besides these,
interrups can be desided by the OS design.

### The IDTR register.

We have the ability to tell the processor where the IDT resisdes in
memory; this is done by the [lidt]{.hljs} (load IDT) instruction; this
works the same way as the [lgdt]{.hljs} instruction, it takes the
operand and loads it into the IDTR register, to later be used to reach
the IDT. The structure of the IDTR is exactly the same as the GDTR, so
we would use this in the same way as we used lgdt.

And that covers just about all the theory we need to know for now, what
a relief, i bet you're happy to begin coding again because i certainly
am.


