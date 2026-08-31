## Making the GDT

before we move into protected mode we must create the GDT. In order to
have a complete GDT we first need 3 descriptors, one is the null
descriptor which is just a 64 bits of 0. The second is the kernel space
code descriptor. The third is the kernel space data descriptor. We could
also make descriptors for the user space, but i will refrain from doing
that for now.

First let\'s define the null descriptor which looks like this:

``` language-x86asm
gdt_start:
gdt_null:
    dq 0
```

Pretty simple, now for the code descriptor

``` language-x86asm
gdt_code:
    dw 0xFFFF ; limit
    dw 0x0000 ; base_low
    db 0x00 ;base_middle
    db 0x9A ;access
    db 0xCF ;flags + limit high 4 bits
    db 0x00 ;base_high
```

#### The base:

The comments i left here are pretty useful. You will remember the
descriptor structure from the previous chapter. We set All of the Base
to 0 because we want the starting address for our kernel space code to
be address 0, this is because with our descriptor we are essentially
making a flat memory model so whenever we reference an address, for
example \[0xB8000\] for writing to the screen with vga, we access this
address using the base + offset (the offset being 0xB8000 which is also
the actual address). So it makes it so we don\'t have to factor in a
base to get to the addresses we want.

#### The limit:

The limit tells us how much memory the segment is allowed to access, we
set it all to F because this is our kernel, so we want it to be able to
access everything that is in memory.

#### The access byte

The access byte tells us what kind of segment we have and who is allowed
to use it. For our 0x9A this translates to 0x9A = 10011010, let\'s look
at the flags:\
`P DPL DPL S E DC RW A`\
`1 0 0 1 1 0 1 0`\

-   **The Present flag (P)** tells us that this segment exists, if it is
    set to 0 and we try to access the segment, the CPU will fault.
-   **The Descriptor Privilege Level (DPL)** is self explanative, we set
    it to 0 because we want the highest privelege.
-   **The Descriptor Type (S)** states what kind of descriptor it is, if
    this is set to 1 it is a normal code or data segment, if 0, it is a
    system descriptor such as a LDT, and we don\'t need that yet
-   **The Executable (E)** when set to 1 means that it is a code
    segment, when zero it represents data
-   **DC stands for Direction/Conforming**When set to 0 it is a
    non-conforming code segment, this means that only code running at
    the correct privelege level may enter it, if DC was 1 then less
    priveleged code may jump into the segment
-   **Read/Write (RW)** when set to 0 means executable only, but it can
    not be read, so code cannot be read as data, so we should set RW to
    1 so it can be executable and readable
-   **The access bit (A)** States whether the segment has been used, it
    is automatically set by the cpu later.

So we can see that the access byte, as the name implies, controls
access.

#### The flags nybble

Our 0xC = 1100, let\'s take a look:\
G D L AVL\
1 1 0 0\

-   **Granularity (G)** being set to 1 makes our limit be measured in
    4KiB blocks rather than bytes
-   **Default Operand Size (D)** being set to one states the segment is
    32 bits instead of 16
-   **Long mode (L)** being set to 0 keeps us in 32 bits, if it was 1 it
    would let us go into 64 bit long mode on x86-64 systems
-   **The Available (AVL)** flag is mostly ignored by the CPU, so let\'s
    just set it to 0

Now let\'s look at the data segment which is pretty similar:

``` language-x86asm
gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92
    db 0xCF
    db 0x00
```

The only difference here is that we turn off the executable flag as this
is a data segment and not a code segment.

The final part of the GDT is we need some memory that we will load into
the global desciptor table registor, this will include the size of the
GDT and the start address. Mine looks like this:

``` language-x86asm
gdt_end:
gdtr:
    dw gdt_end - gdt_start - 1 ; set manually for testing
    dd gdt_start
```

## Going into Protected mode

And that\'s the end of our gdt and all the additional data we need, now
we can start entering the GDT, we can do that with this block of
instructions

``` language-x86asm
enter_protected:
    cli ;disable interrupts
    lgdt [gdtr] ; load GDT registor with start address of GDT
    mov eax, cr0
    or eax, 1 ;set protection enable bit in control register 0 (cr0)
    mov cr0, eax

    CODE_SEG equ gdt_code - gdt_start
    jmp CODE_SEG:p_mode_main
```

This is essentially 3 things, we first use `cli` which disables our bios
interrupts, we then load the descriptor table with `lgdt>` we then set
the protection enable bit in the control registor, after this we then
perform a far jump into the p_mode_main label (which we will define
later) using the code descriptor

``` language-x86asm
p_mode_main:
    mov ax, 10h
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x9000

hang:
    jmp hang
```

This is our code for our p_mode_main, it\'s simple, we just set the data
segment similary to how we did with the `CODE_SEG` earlier, if we wanted
we could replace `CODE_SEG` with 08h. as this would be the value
calculated by `gdt_code - gdt_start` we also load that into all of our
other special segment registers

And that should be all for going into protected mode, our full code
should look like this:

``` language-x86asm
; no org code starts at 0x0900 though
[bits 16]
start:
    mov ax, cs
    mov ds, ax

    mov si, hello_string
    call print_string

    jmp enter_protected

print_string:
    mov ah, 0Eh

print_char:
    lodsb ; sets al = [DS:SI++]

    cmp al, 0
    je done
    
    int 10h

    jmp print_char

done:
    ret

enter_protected:
    cli ;disable interrupts
    lgdt [gdtr] ; load GDT registor with start address of GDT
    mov eax, cr0
    or eax, 1 ;set protection enable bit in control register 0 (cr0)
    mov cr0, eax

    CODE_SEG equ gdt_code - gdt_start
    jmp CODE_SEG:p_mode_main
[bits 32]
p_mode_main:
    mov ax, 10h
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x9000
hang:
    jmp hang

hello_string db 'Hello World!, i am lytlnyblOS, running in real mode', 0

gdt_start:
gdt_null:
    dq 0
gdt_code:
    dw 0xFFFF ; limit
    dw 0x0000 ; base_low
    db 0x00 ;base_middle
    db 0x9A ;access
    db 0xCF ;flags + limit high 4 bits
    db 0x00 ;base_high
gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92
    db 0xCF
    db 0x00
gdt_end:
gdtr:
    dw gdt_end - gdt_start - 1 ; set manually for testing
    dd gdt_start
```

Let\'s take away our -s and -S flags from our makefile and then run the
program and see what happens. What is most likely happening for you is
that it looks like a bunch of text is flashing on the screen this is our
triple fault, if not and the program hangs, you\'re probably in real
mode and can skip what i\'m about to talk about next.

