# Part III : Learning about x86

## The Theory Covered

There's only really four things we need to learn about before
continuing to make our bootloader. And these are the:

-   x86 Operating Modes
-   x86 Memory Segmentation (not to be confused with segmentation in disk drives)
-   The x86 Run-time Stack
-   x86 Interrupts

Let's start, tackling these in order.

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

## x86 Memory Segmentation

What is memory? Well, physically, we can think of memory as just an
array of bytes, each having a memory address that is just a numerical
value stored in base 16; this is our physical view of memory, 
we need a logical view of memory that can make a lot of things much
easier. This is where memory segmentation comes in.

Memory segmentation in x86 architecture is a feature that divides the
memory into segments to allow for more flexible memory management and
protection. Understanding is important when developing an
operating system for x86 platforms, as in protected mode, memory
segmentation is the default and primary mechanism for addressing memory.
In protected mode, memory segmentation is still the default memory
addressing scheme, you can configure it to work
alongside other memory management methods
like paging.

Memory segmentation isn't really used in the modern day; it's an old
way of formulating memory, and it's used on x86 because of physical design
factors of the CPU. Whereas paging is, which we will likely use
in our operating system; we will get into that much later.

Memory segmentation works differently in real mode and protected mode.
Let's look at them individually, starting with a basic overview and
then looking at how it's done in real mode.

## How does memory segmentation work? An overview

First, let's look at a basic overview of how memory segmentation works,
Segmentation is where main memory separates into parts called
segments where Each segment stores related data. To access data inside a
segment, each byte is referred to by its own offset. A running program
is split into 3 different segments in x86; these are:

-   Code segment: Stores code of the program under execution
-   Data segment: Stores the data of the program
-   Stack segment: Stores the data of the program's stack

## How does memory segmentation work in real mode? {#how-does-memory-segmentation-work-in-real-mode style="font-size: 20px;"}

We will start with real mode just so we can be clear without having to
cover all the extra stuff you have to consider in protected mode
(like global descriptor tables). Here in real mode, segmentation
maps by the view of the processor itself, so as said before, there is
no way to avoid it. It's also worth mentioning that each segment in
real mode is 64KB in size. In real mode, we have segment registers with
each register having a size of 16-bits; these registers are:

-   CS: used to define a code segment
-   SS: used to define a stack segment
-   DS: used to define a data segment

There's also other registers that we can use:

-   ES: A segment register that provides flexibility in memory
    access, used when you need to access more segments without
    changing the value of `ds`.
-   GS: global segment register, made to provide a segment for global
    data used similarly to ES
-   FS: file segment register, made to access local thread storage but also 
    used similarly to ES

Each segment register stores the starting memory address of a segment,
we can reach any byte in a segment by setting an offset

Let's look at an example for memory segmentation.

Assume we have some code for a program loaded into memory, which is
stored at 100d. To reach the first byte, we would just set our
offset to 0, and increase it for any next byte we want to access. We
would also set the `cs` register to `100d`, as that is the starting address
for the current code segment we are trying to run.

x86 always runs with memory segmentation in mind, so when we use the `jmp`
instruction, we aren't jumping to a specific space in memory, so let's
say we write `jmp 100d`, we are actually jumping to the offset of 100d
inside the current code segment. This also happens internally with the
PC (program counter), where the PC doesn't Store the full memory
address, just the offset of the next instruction. Any jump to a location
in the same code segment is called a near jump/call; otherwise, it's
called a far jump. To do far jumps, you can do stuff like `jmp
900:1d`, this will load 900d into the CS and 1d into the PC.

This is exactly the same for the other two segments (data and stack); it
was just easy to show using and jump/call because the
functionality is related to code, which is easy to manipulate code flow.
An example for DS would be `lodsb`, and for `ss`, the `push` instruction.

## How was memory segmentation used in the bootloader? {#how-was-memory-segmentation-used-in-the-bootloader style="font-size: 20px;"}

When we wrote the bootloader (and the basic kernel), we
dealt with segments. Let's look at our code. I can now explain it
now that you know everything you need to know about memory segmentation
in real mode.

The first thing we will look at goes all the way back to when we wrote
our printing code together, this is in the start label, here:
```x86asm
    mov ax, 07C0h
    mov ds, ax```

It's worth noting that the CS register is already set to 07C0h
by the bootloader. We also set the same value to the DS register.
This ensures the bootloader can correctly access its own code and data
correctly. But you might ask, "why do we need to load the location into
axe and then `ds`?". This is because we can't directly load into
segment registers due to many limitations on the instruction set, so
we just use axe as an intermediary register to load into `ds`.

Moving on, the next place we used memory segmentation

This is when we were trying to load the kernel into memory from the
bootloader. More specifically, this was when We were trying to use the
13:02h interrupt, which is the interrupt for taking code from storage
into memory. Which we see in this code here:
```x86asm
    load_kernel_from_disk:
    mov ax, 0900h
    mov es, ax

    mov ah, 02h ; service number, 
    mov al, 01h ; number of sectors we want to read from (only simple kernel for now, so less than 512 bytes)

    mov ch, 0h ; number of track we would like to read from, is just 0.
    mov cl, 02h ; sector number that we would like to read its content, this is the second sector

    mov dh, 0h ; the type of disk we would like to read from, 0h means we are reading from a floppy disk. 
    mov dl, 80h ; this is the hard disk we are reading from, 80h means hard disk #0, 81h would be hard disk #1

    mov bx, 0h ; memory adress that content will be loaded into
    int 13h ; 13h provides services related to hard disk
```

Here, what we do first is store 0900h into the extra segment register,
as this is where we will be storing the kernel in memory, you see, the
interrupt 13h:02h loads the content (which we have defined with prior
registers) into the memory address `es:bx` (where `bx` is the offset).

Then after we do that, we can commit a far jump to the next memory
segment where the kernel will be stored. It's worth noting that a far
jump changes the value of the `cs` register to wherever you jump to; in
this case, it's set to 0900h, which is good because that's where
the start of code segment for our kernel is. Then, in our kernel, we can
set the `ds` register to the same as the `cs` to read code and data
from the same segment.

## How does memory segmentation work in protected mode? An intro to the Global Descriptor Table

We have got down how memory segmentation works in real mode, and even
know how it's used in our bootloader. That's pretty good; now we've
just got to cover protected mode, and we're done with memory
segmentation and can move onto the run time stack.

The basics of memory segmentation in protected mode and real mode
are basically the same. But, protected mode comes with some extra
extensions for certain features like memory protection.

In protected mode, we have something called the global descriptor table
(GDT); this is stored in main memory, and its base address is
referenced by the global descriptor table register (GDTR). Just to
clarify, the GDTR is a special register used by the CPU to point to the
location of the GDT in memory.

Each entry in this table is called a segment descriptor; each segment
descriptor has a size of 8 bytes and can be referred to by an index
called a segment selector. The segment selector defines an offset of 8
bytes within the space defined by the GDTR. Each entry in the GDT
defines a segment (of any type) and has the info required by the CPU to
deal with that segment. For instance, the starting memory address of the
segment is stored, and the size/limit of the segment is stored.

Furthermore, as we have this focus around the GDT, our segment registers
from real mode no longer store direct addresses, they store segment
selectors.

#### The structure of the segment descriptor, a basic overview

As we said before, a segment descriptor is an entry of the GDT worth 8
bytes; it's made up of several fields and flags that describe the
attributes of any segment in memory. The processor will then go to the
descriptor that describes the segment when we need to get information
about a segment, like the starting memory address (of said segment). As
well as storing basic info, a segment descriptor stores info that helps
in memory protection; this makes memory segmentation not just a logical
way of viewing memory, but a method of memory protection, protecting
different segments on the system from each other, and not letting less 
privileged segments manipulate data or call code in certain places
(typically more privileged areas of the system).

#### How segments are used when calling and interacting with other memory

The most important information about a segment is its base address (the
starting memory address). In real mode, the base address was stored in
the corresponding segment register directly. But in protected mode, this
is just stored in the segment descriptor.

When currently running, code refers to a memory address to read from or
write to (with data segments) or to call somewhere (with code segments).
It's actually referencing a specific segment in the system and an
offset. This generated memory address that we get when we try to look
somewhere in a different segment is not a physical memory address; it's
actually a logical memory address meaning it doesn't actually
reference the place in which data gets stored; it's simply a logical
representation of where we need to go relative to the program's address
space. In this case, a logical memory address is a segment selector and
offset, to point to the memory location we want to go.

Every logical memory address refers to some byte in the specific segment
in the system, and to actually reference this, it needs translation
into a physical memory address.

In x86, a logical memory address may go through two translation
processes instead of one to receive a physical memory address. The first
step involves turning the logical memory address into a linear memory
address (another non-physical memory address), which is here because of
the paging feature. If the system has paging activated, a second
translation process occurs to turn the linear memory addresses into a
physical memory address; this is because the CPU can view a linear memory address as
a physical address when paging gets disabled. But not when it's
enabled. For now, we will only focus on the process to turn a logical
memory address into a linear memory address.

We know that each logical address consists of two parts, a 16 bit
segment selector and a 32 bit offset. When this is logical address gets
generated by currently running code, the processor then needs to
translate it to get the physical address.

First we read the value of the register GDTR (which contains the
starting physical memory address to locate the descriptor of the GDT),
then we use the segment selector in the logical memory address in order
to locate the descriptor of the segment; this descriptor then contains
the base address of the segment; the processor then obtains this base
address, and adds it to the offset. This provides ups with the linear
memory address.

#### Memory protection in this process, and segment limits

During this process of translation, other information from the segment
descriptor is used to provide memory protection. One of these pieces of
information is called the limit of a segment, This means a segment's
size; if the generated code refers to an offset that exceeds the limit
of a segment, the processor will stop this operation.

The limit of a segment is stored in the 20-bit "segment limit field"
of a segment descriptor; how the processor interprets the value of the
segment limit field depends on the granularity flag (G flag), which is
also stored in the segment's descriptor. When the value of the G flag
is 0, this means the value of the limit field is interpreted as bytes.
So if the G flag is set to 0 and the segment limit field is 20, the size
of the segment is seen as 20 bytes. On the other hand, when it is set to
1, the value of the segment limit field will be interpreted as 4KB
units. To see what this means, assume the value of the limit field is
20, but the G flag is 1. This means that the size of the segment will be
20 of 4KB units, so 20*4 = 80, so the size of the segment is 80KB,
(81920 bytes).

Because the size of the segment limit field is 20 bits, this means that
the max numeric value it can represent is 2^20, this means that is the
G flag is 0, the maximum size is 1MB, and if it's set to 1, the maximum
size is 4 GB.

#### Back to the structure of the descriptor, looking more in depth

I can show you the complete structure of a descriptor using a diagram
taken from the "Intel® 64 and IA-32 Architectures Software Developer's
Manual (Volume 3A)", seen here:

![Segment Descriptor](images/os/segdescriptor.png)

The first 16 bits (bit 0-15) are the first 16 bits of the segment's
limit. then the next 24 are the first 24 bits of the segment's base,
then we have our type field, S flag, DPL field, P flag, Then we have the
next nibble of our limit, the AVL flag, the L flag (for 64 bit), the DB
flag, the G flag, and the next section of our base.

You may be wondering, why is the segment descriptor formatted so
strangely? and it's simply because of reverse compatibility with the
80286 16-bit microprocessor; here is a similar diagram seen from the
Intel 80286 Programmer's Reference Manual:

![Old Descriptor](images/os/olddescriptor.png)

On the 80286 diagram, the base size was 24 bits, and the limit's size
was 16 bits, therefore we just extend this for our newer proccessor
architecture.

#### A segment's type

When a segment is defined, the processor should know how to interpret
the content inside this segment; this is defined by the type of segment.
We know so far that there are code segments. and data segments, these
two types belong to a category of segments called application segments;
there is another category called system segments, and many types of
segments belong to it.

Whether a specific segment is an application or system segment, is
defined in the S flag, also known as the descriptor type flag, which is
the fifth bit in the fifth byte of the segment descriptor. When the S
flag is 0, the segment is considered a system segment; when it is an
application segment, the value of S is 1. We will focus on when the S
flag is 1.

The only application segments are code and data. If some application
segment is referenced by currently running code, the processor will go
to the descriptor of this segment. and by reading the S flag (which
should be 1), it should know that the segment in question is an
application segment, but how does it know whether it's a data or code
segment? This info is stored in a field called the type field in the
segment descriptor.

The type field is the first 4 bits of the 5th byte of the segment
descriptor. The most significant bit specifies if the application
segment is a code or data segment; the least significant specifies
whether the segment has been accessed or not; When the value of this is
1, this means that the segment has been written to or read from, but if
it's 0, this means that the segment has not been accessed. The value of
this bit is changed by the processor in only one situation; this is when
the selector of the segment is loaded into the GDT. In any other
situation, It's up to the OS to decide the value of the accessed flag.
According to Intel, this flag can be used for virtual memory management
and debugging.

The other two bits or flags of the type field depend on whether it's a
code or data segment. So let's cover those individually.

#### The type field for Code segments

When the segment is a code segment, the second most significant bit of
the type field is called the conforming flag (C flag), whereas the third
most significant bit is called the read-enabled (R flag), starting with
the simplest being the R flag.

The value of this flag indicates how the code inside the segment can be
used, when the value of the R flag is 1, this means the content of the
code can be executed and read from, but when it's 0, this means that is
can be only be executed, and not read from.

The conforming flag is all to do with privilege levels. When a segment
is conforming (the value of the conforming flag is 1), this means that
code that runs at a less-privileged level can call this segment, which
runs in at a higher privilege level. Why would we want this? Well, the
kernel can sometimes provide code that is basic and may be needed by
many programs. This code would have a privilege level of 0, as it is a
part of the kernel and would gain the highest privilege level; however,
any other programs, which would have a lower privilege level wouldn't
really be able to call this without the conforming flag. So this is why
it is needed.

#### The type field for Data segments

Now, when we are working with data segments, the second most significant
bit is called the expansion-direction flag (E flag), and the third most
significant is called the write-enabled flag. (W flag)

The write-enabled flag gives us the ability to decide whether we want
our data segment to be read-only or not; when set to 0, the data will be
read-only; when set to 1, the data segment will be both. readable and
writable

The expansion-direction flag will be covered when I move onto the x86
run-time stack. For a vague definition now, we could say that when the
value of the flag is 0, the data segment is going to expand up, but when
the value of the flag is 1, the data segment will expand down. (These
are Intel's terms, so don't blame me).

An extra thing about data segments is that all of them are
non-conforming, which means less priveleged code cannot access data at a
more priveleged level, and all data segments can be accessed by a
more-privileged code.

#### Privelege levels in segments

Prior, I have probably stated that a segment should belong to a
privilege level, and based on this, there are rules for how certain
segments can interact based on these privilege levels. which the
processor would enforce; these privilege levels are defined by the
descriptor privelege level (DPL) in the segment descriptor; as this is a
2 bit value, the possible privilege levels are 0, 1, 2, and 3, the DPL
is the second and third most significant bits of byte 5 in a descriptor.

#### The other flags: The D/B flag

There are only 3 flags left that I haven't covered. The first is a flag
whose name changes depending on the segment it resides within; it is
located within the second. most significant bit in byte 6 when it within
a code segment, it is called the default operation size flag (D flag),
When the processor executes the instructions, it uses the D flag to
choose the length of the operands, depending on the currently executing
instruction. If the value of the D flag is 1, the processor is going to
assume the operand has a size of 32 bits if it's a memory address, and
32 bits or 8 bits if it's not a memory address, when the value of the D
flag is 0, and the processor is going to assume the operand has the size
of 16 bits if it's a memory address and 16 or 8 bits if it's not a
memory address.

When the segment is a stack segment, the same flag is called the default
stack pointer size flag (B flag), and it decides the size of the memory
address that points to the stack. which is commonly known as the stack
pointer, used by stack instructions such as push and pop. When the value
of the B flag is 1, then the size of the stack pointer will be 32 bits,
and it's value will be stored in the register ESP. When the value of
the B flag is 0, the size of the stack pointer will be 16 bits, and
it's value will be stored. in the register SP

When dealing with a data segment that grows upward, this is called an
upper bound flag (B flag); when it's value is 1, the maximum possible
size of the segment will be 4 GB; otherwise, the maximum size is 64KB

To note, the value of the D/B flag should usually be 1 for 32 bit
segments and 0 for 16 bit segments.

#### The other flags: The L flag

This is known as the 64-bit code segment flag (L flag), which is the
third most significant bit in the byte 6. If the value of this flag is
1, that means the code inside this segment is 64-bit code, while 0 means
the opposite; when the value of the L flag is 1, the D/B flag should be
0.

#### The other flags: The AVL flag

This flag doensn't really have any paticular meaning for the processor,
however, this flag is availible for the OS to use in whatever way it
needs, or it is just ignored.

And that wraps up all coverage of the descriptor, moving on.

#### More on the GDTR

As we know, the GDTR stores the base (physical) address of the global
descriptor table, but it also stores the limit of the table. To load a
value into the register of the GDTR, follow the instruction:
[lgdt]{.hljs} must be used; this stands for "load global descriptor
table." It takes one operand, which is the value that should be loaded
into the GDTR. This operand's structure should be similar to the actual
structure of the GDTR, which is shown here:

![GDTR Diagram](images/os/GDTR%20diagram.png)

We can see it's 48 bits long, starting with the 16-bit limit, and then
the 32-bit base. The operand should follow this same formula. This also
means that we have some limits to our GDTR (no pun intended), as our
limit is a 16-bit number, the maximum value of our GDT. is 64KB (65536)
bytes.

#### The local descriptor table

The GDT is system-wide; this means that it is available to every single
process within the system. x86 also gives us the power to create local
descriptor tables (LDTs) in protected-mode, These have the same
functionality and structure as the GDT. Multiple of these LDT's can be
made; each one can be private to a specific process currently running on
the system; multiple processes can also share a single LDT that only
those processes can interact with, and no other processes will be able
to interact with this LDT.

How to use an LDT depends on how the kernel is being designed; whereas
the GDT is required by x86 architecture, the LDT is optional and in the
hands of the designer. To tell the processor that a given region of
memory is in an LDT, a new segment descriptor should be created in the
GDT for the LDT; the LDT table will be considered as a system segment,
so the value of the S flag would be 0, and because there are many
different system segments in x86, we would then have to define that this
is an LDT. This is done via the type field, and its value should be
0010b. How the processor can tell which table should be used in the
monment for a given segment between the GDT and the LDT will be
discussed when we talk about segment selectors.

The x86 instruction lldt is used to load the LDT table that we would
like to use now into a special register called the LDTR, which is 16-bit
and contains the index of the segment. descriptor, which describes the
LDT table.

#### More on the segment selector

In reality, the way we described the segment selector before as an
index, is not actually true; the index is simply just a single part of
the segment selector. A full diagram of it can be seen here:

![Segment Selector](images/os/segmentselector.png)

We can see that it is 16 bits, and the first two bits are occupied by a
field called the "requester privilege level" (RPL). The next bit is
then occupied by a flag called the table indicator, and then we have our
usual index field, which we know much about.

The TI flag is used by the processor to tell if the index in the segment
selector is an index in the GDT or the LDT; when it is at 0, the index
signifies the GDT; when it is at 1, it signifies an LDT; when it's 1,
the processor consults the LDTR to know which LDT we are working with,
and then the descriptor on the LDT is read.

The RPL, as the name suggests, is to do with privilege levels; we
mentioned before the DPL, the privilege level of a given segment, and
there also exists the CPL. which is the privilege level of the currently
executing code, The RPL is used when a low privilege program calls,
let's say, kernel data that is a more high privilege. When calling, the
RPL defines the privilege level of the caller, so any attempts to reach
a segment where its sector RPL is larger than the CPL should be denied.

## x86 Run-Time Stack

I'll assume you already know how the stack data structure works
regularly without a context as it's one of the most basic data
sructures in computer science, if you don't, do not worry, as it's
super simple and there are many great explanations
[online](https://www.geeksforgeeks.org/introduction-to-stack-data-structure-and-algorithm-tutorials/)

### The implementation

The reason we need the run time stack is to implement the lifecycle of
functions, so what happens when we call and return from a function,
etc.? We know that a program usually consists of many subroutines
(functions), and all of these subroutines fill out a specific goal
within the program. Let's say a function called *y* starts it's life
when called by another function: *x*, *y* is the callee, and *x* is the
caller. The callee can define its own local variables, which are
private, and these variables can be removed from memory once the callee
returns.

So, when the callee returns, the processor needs to know when it
finishes and the location of the code that we should return to (which is
usually right after the function call)

In x86, each process will have it's own run-time stack; this is a
memory region that obeys the rules of the stack data structure. The run
time-stack is divided into multiple small stacks; these are called
"stack frames." Each stack frame relates to a function that has been
called during execution, once the function ends it is removed from the
larger run-time stack, therefore removing it from memory.

The register [EBP]{.hljs} (stack frame base pointer) contains the
starting memory address of the current stack frame, the register
[ESP]{.hljs} contains the memory address of the top of the stack. To
push a new item to the stack, the [push]{.hljs} instruction can be used,
where the operand is the new item. will decrement the value of ESP to
get a new starting address for the next value; this means that the value
of the x86 run time stack grows down in memory.

The [pop]{.hljs} instruction can also be used; this will increment the
value of the ESP and store the value that was on the top of the stack to
the memory location. or register specified by the operand. It also
won't overwrite the popped value with null data; it will just leave it.
This is for performance, as writing a bunch of 0's in the place would
take up unnecessary recourses. when we can just change the existing data
when overwriting in the future.

#### The cdecl calling convention

When a function needs to call another, the caller should push all the
parameters that should be passed to be into the callee to the stack. So,
the callee's parameters will be on A's stack. frame. It's also worth
noting that parameters are pushed in revered order, so parameter 2 will
appear below parameter 1 on the stack frame. After this, the
[call]{.hljs} instruction is used to start running the code of the other
function, but before jumping, the instruction pushes the value of the
return address onto the stack, which would be stored in the program
counter (EIP), this is done so we know where to go next after we are
done executing our code.

When a function starts in a program, it is responsible to creating its
own stack frame, so the first thing a function should have in it's code
is the code that makes a new stack frame. A function does this by
pushing the current value of EBP to the stack frame; this is because
it's going to be changed in a second. then we move the value of ESP
(stack pointer) to EBP (starting address of the current stack frame). So
now, we can continue working with our function and it's stack pointer
now, pushing any value we may need, etc..

You may be thinking, "But how do we reach our parameters if they are
further down the stack?" Well, this is where the EBP register comes in,
as we can use this to access our parameters. by referencing an
incremented value of it, which would always be the same as only the
prior EBP and return address are stored between the current value of the
EBP and the required parameters.

It's also worth noting that all values pushed to the stack are worth 4
bytes, as we are in 32-bit protected mode. So to reach a parameter using
EBP, we can do EBP + 8 as the value of the EBP, which moves in bytes.
You may think we need to add 12 to reach the starting address of the
first parameter. but remember the stack grows downwards, and the data
would be stored upwards.

When the callee needs to return any sort of value, we can just store it
in a register like EAX, and then to actually return, we can just pop all
the values from the stack. until we get to the prior value of EBP, this
can then be popped into the actual EBP register. Then at the top of the
stack is the memory address we need to return to, So this can just be
loaded into the program counter; this can be done using the x86
instruction called [RET]{.hljs} (which will also pop the value so we
will not need to pop it ourselves.)

Then, when our caller gets control again, we can pop our previous
parameters, just so we can clear up some memory space for the remainder
of the function. You would expect to use pop. But it can be done just by
incrementing the stack pointer by 4, and this deallocates the memory
without having to store it somewhere (as pop stores the values and
removes them).

The implementation of calling and returning from functions is not
written in stone for x86; it is simply a convention; this one is known
as cdecl, or the C declaration. There are many other conventions, and
you can even make/design your own.

#### Growth direction of the Stack

When I state that a stack is growing downwards, this simply means that
the newer items being added to the stack have smaller memory addresses
than the prior ones. The bottom of the stack has a larger memory address
than the top of the stack.

The stack growing downwards is done by default in x86; there are also
visualisations where the stack grows upward that are used in other
systems. The reason why stacks usually grow downwards with x86, which is
also probably due to historical design factors, but you could actually
make the x86 run time stack grow upward if you wanted to, but for now,
I'll just stick with letting it grow downwards; you may remember the
expansion-direction flag I left out for when we covered stacks. Well,
when it's value is 0, the stack will grow downwards, and to grow
upwards, it's value should be 1.

#### Advantages and disadvantages of growing upwards and downward

Downwards stack growth is mostly used because of many reasons, the most
prominent being from early computing when memory was limited, so it
would not be good to pre-allocate a large chunk of memory to exclusively
be used by the stack, down growing fixed this, providing an efficient
use of memory. After this, most architecture just would have maintained
compatibility with previous version of the same architecture.

The upwards growing stack allows us to resize the stack, at the cost of
inefficient memory usage (this also has to do with the stack and the
heap growing in opposite directions). But I haven't talked about the
heap that much, so don't really worry about it.

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

