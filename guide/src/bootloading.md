# Bootloading

## A few words

If you love coding, you're probably eager to get straight into the programming and make something that functions. 
Well, good news. In this chapter, we will end with a fully functioning bootloader that loads a simple kernel that prints 
a "hello world" message. 
How exciting!

This also shouldn't take that many lines of code, although the instructions can be complex, I will try to explain to the best of my ability in this chapter.

Sadly, if you like hands-on experience, the next chapter will cover a lot of theory, but I will try to make it as concise 
as possible, 
as I only try to cover key points that you can research later if you have any interest.

## Printing in BIOS

To show how things work through this series of articles, I will be providing a block of code and then 
explaining it further. 
After the block, I will also be providing some comments on ambiguous lines of code.

Here's some basic code I made for printing, which we will be expanding upon after an explanation, to do things like reading 
from storage and loading into memory:

```x86asm
start:
    mov ax, 07C0h
    mov ds, ax

    mov si, title_string
    call print_string
    jmp $

print_string:
    mov ah, 0Eh ; bios number 0Eh, sets for teletype output function
print_char:
    lodsb ; loads byte at SI, into AL and increments SI

    cmp al, 0 ; 0 stored in al if at end of string
    je printing_finished

    int 10h ;bios interrupt 0x10, to print char stored in AL
    jmp print_char
printing_finished:
    ret

title_string db 'Welcome to the lytlnybl bootloader!',0

times 510-($-$$) db 0 ; pads the rest of the bootloader with 510 bytes, aiming for a 512 byte bootloader
dw 0xAA55 ; specifies the end of the bootloader, recognised by bios
```

### The `start` section:

Just a warning: there are a lot of things you might have to take my word on, 
as there will be things we need to do that I can't explain in a couple of sentences and would require a lot of explanation, 
but I will try to answer all the things that I have tried to make you take my word for in the more theory-focused sections.

Now, the first two lines of code (inside the start section) are things that I cannot provide that much context on due to
being entangled with a wider concept in x86 programming. But I will briefly explain here.

```x86asm
mov ax, 07C0h
mov ds, ax
```

With these two lines, the first line loads the value `0x07C0` into the `ax` register; 
this is the segment value we use to access the bootloader at physical address `0x7C00`. 
This is something we will further cover when we go over x86 segmentation, 
as that's a large topic in x86.

Just after that, we set `ds` to the value in `ax`. This will set the `ds` register, which represents the data segment, 
to the value of `ax`, ensuring that the access through `ds` refers to the bootloader's data. 
If we didn't include this line, `ds` could refer to a different segment, causing `mov si, title_string` and
`lodsb` to access the wrong memory.
I will cover this in more detail in the next segment of articles. (pun not intended)

This just about wraps up the hardest part to understand of our printing program, 
and it's only hard to understand because we don't have the required knowledge 
to comprehend our reasons for doing what we are doing.

Continuing with the start section, we then have our next couple of lines, these being:

```x86asm
mov si, title_string
call print_string
jmp $
```

`mov si, title_string`: this first line is what tells us what string we need to print,
Later in the code, we use `db` (define byte) to store our string in memory, as seen here:
`title_string db 'Welcome to the lytlnybl bootloader!', 0` although you might suspect, like in a lot
of programming languages, that we would just be passing the whole string to the `si` register.
That is not what is happening; instead, we are passing the memory location of the first character in the string.
This is because, like in C, strings are treated as arrays, which is terminated by a zero byte to mark the end 
of the string, when we pass the first memory location, we would then increment this and print one character at a time
when we go into our printing sections.

These next two lines are simple: `call print_string` and `jmp $`,
We first call `print_string`, which returns here after printing. The next line jumps to itself, creating an 
infinite loop.

### The `print_string` section:

Let's just look at the whole of our print section, and I can explain it all in the
coming paragraphs, so our print section goes as follows:

```x86asm
print_string:
    mov ah, 0Eh ; bios number 0Eh, sets for teletype output function
print_char:
    lodsb ; loads byte at SI, into AL and increments SI
            
    cmp al, 0 ; 0 stored in al if at end of string
    je printing_finished
            
    int 10h ;bios interrupt 0x10, to print char stored in AL
    jmp print_char
printing_finished:
    ret
```

The first line `mov ah, 0Eh` moves the value `0Eh` into the `ah` register.
In BIOS interrupt services, `ah` typically specifies the function requested. In this case,
`0Eh` is the function number for teletype output.

Then we enter the `print_char` loop, which repeats until the end of the string.

The `lodsb` instruction (as said in the comment) loads the byte at the memory address
pointed to by the `ds:si` (which in the beginning would be the first character) into the
`al` register, and then increments the `si` register to point to the next byte in memory.

Next we have `cmp al, 0` and `je printing_finished`
The first instruction compares the value in the `al` register
with `0`, checking if it's the null terminator, which marks the end of the string.
After that, we use `je` (which means jump if equal) instruction to jump to `printing_finished`
if the value in `al` is the null terminator, which is 0.

Then, simply, if we are not at the end of our string, we carry out our final
instructions: `int 10h` and `jmp print_char`.
The first of the two invokes the BIOS interrupt `10h` which
is the BIOS interrupt for video services, and in this case, the value in `ah`
is `0Eh` indicating a teletype output, so the byte in `al` is 
interpreted as an ASCII character and is then printed to the screen.

Then, in our final section, we return to the caller

### Final Two lines

I'll now explain the last two lines, which may look a little confusing. These are:

```x86asm
times 510-($-$$) db 0
dw 0xAA55
```

The first line tells the assembler to add enough zeroes to make
the bootloader 510 bytes long. This ensures that the
bootloader fills up most of the available space in the 512-byte sector reserved
for the bootloader.
The next line adds the boot signature, `0xAA55`, to the final two bytes of the sector.
Which the BIOS uses to recognize the sector is bootable.
When the BIOS loads the bootloader, it checks for this signature
to make sure it's a legitimate bootable sector before proceeding with the boot process.

That wraps up our printing, We use printing code a couple of times in the BIOS.
Although I'm pretty sure when we move out of the BIOS, we will have to print differently, 
as we will not have access to the BIOS teletype services.
Now, it's time to move onto the next section, which will load our kernel into memory.

Before we move on, here is the Makefile for our code:

```makefile
# Assembler
NASM := nasm

# Assembler flags
NASMFLAGS := -f bin

# Source files
SRC := print.asm

build: $(SRC)
    $(NASM) $(NASMFLAGS) -o print.o $(SRC)
    dd if=print.o of=print.img
    qemu-system-x86_64 print.img
    rm -f print.o

clean:
    rm -f *.o *.img
```

## Loading the kernel

To load the kernel, we actually need a kernel to load. Here's a simple one:

```x86asm
start:
    mov ax, cs
    mov ds, ax

    mov si, hello_string
    call print_string

    jmp $

print_string:
    mov ah, 0Eh

print_char:
    lodsb

    cmp al, 0
    je done
    
    int 10h

    jmp print_char

done:
    ret

    hello_string db 'Hello World!, i am lytlnyblOS', 0
```

This code is close to the printing code we wrote earlier, with the main difference being how we set up the data segment;
instead of setting `ds` to a fixed segment value, we copy the current code segment from `cs` into `ds`. This makes `ds`
and `cs` refer to the same segment.

After focusing that much on printing earlier, this should be straightforward. 
Now let's change the bootloader to accommodate 
our new kernel, and I will then explain the need for the updated lines:

```x86asm
start:
    mov ax, 07C0h
    mov ds, ax

    mov si, title_string
    call print_string

    mov si, message_string
    call print_string

    call load_kernel_from_disk
    jmp 0900h:0000 ; gives control to the kernel by jumping to its starting point.

load_kernel_from_disk:
    mov ax, 0900h
    mov es, ax
    
    mov ah, 02h ; service number, BIOS read-sector function
    mov al, 01h ; number of sectors we want to read from (only simple kernel for now, so less than 512 bytes)
    
    mov ch, 0h ; track number we would like to read from, is just 0.
    mov cl, 02h ; sector number that we would like to read its content, this is the second sector

    mov dh, 0h ; head number 0 
    mov dl, 80h ; BIOS drive number: 80h is the first hard disk

    mov bx, 0h ; memory adress that content will be loaded into
    int 13h ; 13h provides services related to hard disk

    ; INT 13h clears carry flag on success and sets it on error.
    jc kernel_load_error

    ret

kernel_load_error:
    mov si, load_error_string
    call print_string

    jmp $

print_string:
    mov ah, 0Eh ; bios number 0Eh, sets for teletype output function
print_char:
    lodsb ; loads byte at SI, into AL and increments SI

    cmp al, 0 ; 0 stored in al if at end of string
    je printing_finished

    int 10h ;bios interrupt 0x10, to print char stored in AL

    jmp print_char
printing_finished:
    ;print new line
    mov al, 10d ; ASCII code for new line
    int 10h 

    ;read current cursor position
    mov ah, 03h ; function to read cursor position
    mov bh, 0 ; page number 0 for default page
    int 10h ; 10h now used to read cursor position

    ;move cursor to beggining
    mov ah, 02h ; function to set cursor position
    mov dl, 0 ; column number (0 for begginign of line)
    int 10h ; 0x10 to set cursor pos

    ret

title_string db 'Welcome to the lytlnybl bootloader!',0
message_string db 'Loading up the kernel for you...',0
load_error_string db 'Oh oh!, there was a problem loading the kernel',0

times 510-($-$$) db 0 ; pads the rest of the bootloader with 510 bytes, aiming for a 512 byte bootloader
dw 0xAA55 ; specifies the end of the bootloader, recognised by bios
```

There's not that much that is new, but this is where most of the code is that I can only explain on a surface level.
I will explain it all as best as I can now in order of logical operation, and then in later chapters we can cover these x86
concepts in the next chapter of this series.

### The `load_kernel_from_disk` section:

Carrying on in order of logical operation, after we print two times (for loading and intro messages), we go straight
into our section for loading the kernel from disk. Its goal is to read the kernel from the disk and load it into memory.

We first set the segment address to `0900h` by loading it into `ax` and then copying it to `es`. The BIOS will load
the kernel at `ES:BX`

Next, we set the disk read parameters with `mov ah, 02h` and `mov al, 01h`.
`ah = 02h` selects the BIOS read-sectors function, while `al = 01h` tells the BIOS to read 
one sector.

The next lines, `mov ch, 00h` and `mov cl, 02h` set the cylinder and sector we want to read. `ch` contains 
the low 8 bits of the cylinder number, so setting it to `0` selects cylinder 0.
`cl` contains the sector number in its lower 6 bits, so setting it to `2` selects the second sector.

We then specify the disk and head with `mov dh, 0` and `mov dl, 80h`. `dh` selects head 0,
while `dl` contains the BIOS drive number. `80h` selects the first hard disk, while `81h` selects the second.

Then `mov bx, 0h` sets the offset within the `es` segment where we will load the kernel, which will just be 0
as we want to load it into the start of our segment.

Our final line is `int 13h`, which invokes the BIOS disk services using the parameters we set int he registers

Then the only thing left to do is check for errors; the interrupt earlier would set the carry flag if there was an error.
So we can just use `jc` (jump if carry) to jump to an error handling subroutine, which will just output a message signifying
an error and then create an infinite loop.

That's all on reading from the disk; let's now look at the changes that we made to printing, which only really
allow us to print multiple lines.

### Printing Changes:

The only real changes to printing made in our code are the changes to the `printing_finished` section of our code, as
seen here:

```x86asm
printing_finished:
    ;print new line
    mov al, 10d ; ASCII code for new line
    int 10h 

    ;read current cursor position
    mov ah, 03h ; function to read cursor position
    mov bh, 0 ; page number 0 for default page
    int 10h ; 10h now used to read cursor position

    ;move cursor to beggining
    mov ah, 02h ; function to set cursor position
    mov dl, 0 ; column number (0 for begginign of line)
    int 10h ; 0x10 to set cursor pos

    ret
```

All this code is pretty basic; we first output the ASCII line feed (`10`) which advances the cursor to the next row.
We then read the cursor position and reset the column to 0.
Next, we read the current cursor position. This is not strictly necessary, but it gives us the current row in 
`dh` and column in `dl`. We then reset the column to `0` while keeping the current row.
The final block moves the cursor to column 0 on the current row, which is also explained in the comments for the code.

There we have it. After writing all this, you can say you've made your own operating system (albeit a simple one).
It's a bootloader that loads a kernel, which then outputs a message, which may seem pretty dull, but just consider the fact
that this was all done on bare metal hardware without an OS to support us, which is pretty cool.

Here is the Makefile for the kernel and bootloader:

```makefile
BOOT_FILE = bootloader/bootloader.asm 
KERNEL_FILE = kernel/basic_kernel.asm 
        
build: $(BOOT_FILE) $(KERNEL_FILE)
    nasm -f bin $(BOOT_FILE) -o bootstrap.o
    nasm -f bin $(KERNEL_FILE) -o kernel.o
    dd if=bootstrap.o of=kernel.img
    dd seek=1 conv=sync if=kernel.o of=kernel.img bs=512
    qemu-system-x86_64 kernel.img
            
clean:
    rm -f *.o
```

