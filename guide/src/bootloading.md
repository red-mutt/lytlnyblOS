# Bootloading

## A few words

In this chapter, we will write a fully functioning bootloader that loads a kernel that prints 
a "hello world" message. I want to start you off with this so you can get a feel 
for bare metal programming before you invest any more time into learning theory in part 3.

A downside to starting with programming is that you will have to take my word
for a lot of the instructions we write without understanding every single one. You will understand 
everything written here after "Part III: Learning about x86." I will give explanations in this chapter,
but they will be missing context and will be large oversimplifications. Please don't get discouraged
if you read something and don't understand it in this chapter.

As a general rule in this book: each chapter will start off with a collection of context that you can use
to develop your own implementation before you look at my code (with the sole exception of this one). 
I highly recommend you try things out before using
my solutions. You will learn much more if you do so. My code has also
not cleaned thoroughly, so you should treat it as a rough guide and not as gospel.

## How does the bootloader start?

Before any code gets written, you need to understand how the first
instructions we write actually get executed. When we run QEMU with our disk image,
QEMU emulates a computer starting up. The virtual machine begins by running its firmware,
which in this case provides the BIOS.

The BIOS performs some basic hardware initialization and then looks for a bootable
device. Our `print.img`/`kernel.img` is being used as that device. The BIOS reads
the first sector of the image into memory at address `0x7C00` and checks that the final two
bytes contain the boot signature `0xAA55`.

If the signature is present, the BIOS treats the sector as a boot sector
and transfers control to the code that was loaded at `0x7C00`. This means that the
`start` label in our assembly code is where our bootloader begins executing.

We don't really need to know how exactly the BIOS performs the operations spoken about.
We only need to know that QEMU provides the virtual machine, the BIOS starts the machine
and loads our boot sector, and then the CPU begins executing our code.

## Printing in BIOS

When printing a string in BIOS, you need to print the string one character at a time in a loop.
This is a common practice in assembly where you put the address of the string in the `SI` register,
read a character from `[SI]`, check whether you've reached the end, send the character to whatever output
mechanism is used, increment SI, and then repeat.
The output mechanism is provided by the BIOS in the form of an interrupt.

Here's some basic code I made for printing, which we will be expanding upon later to do things like reading 
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

These first two lines are an example of something you will have to take my word on, as they
relate to a larger topic called memory segmentation.

```x86asm
mov ax, 07C0h
mov ds, ax
```

With these two lines, the first line loads the value `0x07C0` into the `ax` register; 
this is the segment value we use to access the bootloader at physical address `0x7C00`. 
You will understand this fully when we cover x86 segmentation, but all you need to know
for now is that it makes it so all data accessed is accessed around the `0x7C00` (not `0x07C0`) 
address.

Just after that, we set `ds` to the value in `ax`. This will set the `ds` register, which represents the data segment.
If we didn't include this line, `ds` could refer to a different segment, causing `mov si, title_string` and
`lodsb` to access the wrong memory.

Continuing with the start section, we then have our next couple of lines, these being:

```x86asm
mov si, title_string
call print_string
jmp $
```

`mov si, title_string` is what tells us what string we need to print.
We also use `db` (define byte) to store our string in memory, as seen here:
`title_string db 'Welcome to the lytlnybl bootloader!', 0`. 
By setting `SI` to `title_string` the value in SI is the memory location of the first character in the string.
Like in C, strings are treated as arrays, which are terminated by a zero byte to mark the end 
of the string. We would then increment `SI` and print one character at a time
when we go into our printing sections.

`call print_string`
calls the `print_string` label and returns after printing. `jmp $` jumps to itself, creating an 
infinite loop and marking the end of the program.

### The `print_string` section:

Let's just look at the whole of our print section:

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

`mov ah, 0Eh` moves the value `0Eh` into the `ah` register.
In BIOS interrupt services, `ah` typically specifies the function requested. In this case,
`0Eh` is the function number for teletype output.

Then we enter the `print_char` loop, which repeats until the end of the string.

The `lodsb` instruction (as said in the comment) loads the byte at the memory address
pointed to by the `ds:si` (which means `0x7C00` + `SI` offset/address) into the
`al` register. Then it increments the `si` register to point to the next byte in memory.

Next we have `cmp al, 0` and `je printing_finished`.
The first instruction compares the value in the `al` register
with `0`, checking if it's the null terminator.
After that, we use `je` (which means jump if equal) to jump to `printing_finished`
if the value in `al` is the null terminator.

Then, if we are not at the end of our string, we carry out our final
instructions: `int 10h` and `jmp print_char`.
The first of the two invokes the BIOS interrupt for video services: `10h`.
The value in `AH`
is `0Eh` indicating a teletype output which makes the byte in `AL` be 
interpreted as an ASCII character which is then printed to the screen.

Then, in `printing_finished`, we return to the caller

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
This tells the BIOS uses to recognize the sector is bootable.
When the BIOS loads the bootloader, it checks for this signature
to make sure it's a legitimate bootable sector before proceeding with the boot process.

That wraps up our printing, We use printing code a couple of times in the BIOS.
Although when we move into C, printing will be a lot easier.
Now, it's time to move onto the next section and load our kernel into memory.

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

The kernel code is close to the printing code we wrote earlier, with the main difference being how we set up the data segment;
instead of setting `ds` to a fixed segment value, we copy the current code segment from `cs` into `ds`. This makes `ds`
and `cs` refer to the same segment.

Now let's change the bootloader to accommodate 
our new kernel:

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

There's not that much that is new. Remember that if you don't understand much fret not as it will
get explained in the next part of this book.

### The `load_kernel_from_disk` section:

After we print two times (for loading and intro messages), we go straight
into our label for loading the kernel from disk. Its goal is to read the kernel from the disk and load it into memory.

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
We can just use `jc` (jump if carry) to jump to an error handling subroutine, which will just output a message signifying
an error with an infinite loop.

That's all on reading from the disk; let's now look at the changes that we made to printing, which 
allows us to print multiple lines.

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

We first output the ASCII line feed (`10`) which advances the cursor to the next row.
Next, we read the cursor position and reset the column to 0.
After that, we read the current cursor position. This is not strictly necessary, but it gives us the current row in 
`dh` and column in `dl`. We then reset the column to `0` while keeping the current row.
The final block moves the cursor to column 0 on the current row, which is also explained in the comments for the code.

There we have it. After writing all this, you can say you've made your own bootloader and kernel (albeit simple ones).
This may seem pretty dull, but just consider the fact
that this was all done on bare metal hardware without an OS to support us.

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

