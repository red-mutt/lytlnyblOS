# Part II : Bootloading

## A few words...

If you love coding, you're probably eager to get straight into the programming and make something that functions. 
Well, good news! In this chapter, we will end with a fully functioning bootloader that loads into a simple kernel that just loads a hello-world message. 
How exciting!

This also shouldn't take that many lines of code, although the instructions can be very complex, which I will try to explain to the best of my ability in this chapter.

Sadly, if you like hands-on experience, the next chapter will cover a lot of theory, but I will try to make it as concise and quick as possible, 
as I only try to cover key points that you can research later if you have any interest.

## Printing in BIOS

Mainly, to demonstrate how things work through this series of articles, I will be providing a block of code and then explaining it further. 
After the block is provided, I will also be providing some comments on relatively ambigious lines of code.

Here's some basic code I made for printing, which we will be expanding upon after an explanation, to implement stuff like reading from storage and loading into memory:

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

### The "start" section:

Just a warning: there are a lot of things you might have to take my word for, 
as there will be things we need to do that cannot be explained in a couple of sentences and would take quite a lot of explanation, 
but I will try to answer all the things that I have tried to make you take my word for in the more theory-focused sections.

Now, the first two lines of code (inside of the start section) are things that I cannot provide that much context on due to 
their being entangled with a wide concept in x86 programming. But I will briefly explain here.

```x86asm
mov ax, 07C0h
mov ds, ax
```

With these two lines, the first line loads the value `0x07C0` into the ax register; 
this is the segment where the bootloader code is loaded by the bios. This is something we will further cover when we go over x86 segmentation, 
as that's a large topic in x86.

Just after that, we set ds to the value in ax, This will set the ds register, which represents the data segment, 
to the value of ax, ensuring that memory accesses within the bootloader use the correct segment. If we didn't include this line, 
later, when we would try to print characters, we might notice that our code would just be printing a random amount or no characters at all. 
This will be because it will be reading from memory addresses not relative to the segment that the bootloader code is running in; 
this will also be covered in more detail in the next segment of articles. (pun not intended)

This just about wraps up the hardest part to understand of our printing program, 
and it's only hard to understand because we don't have the required knowledge 
needed to comprehend our reasons for doing what we are doing.

Continuing with the start section, we then have our next couple of lines, these being:

```x86asm
mov si, title_string
call print_string
jmp $
```

`mov si, title_string`, this first line is what tells us what string we need to print,
Later on in the code, we use `db` (define byte) to store our string in memory, as seen here:
`title_string db 'Welcome to the lytlnybl bootloader!', 0` although you might suspect, like in a lot
of programming languages, that we would just be passing the whole of the string to the `si` register.
But that is not what is happening; instead, we are passing the memory location of the first character in the print_string.
This is because, like in C, strings are treated as arrays, which is evident with the null pointer, which shows that we are at the end.
of our string, when we pass the first memory location, we would then increment this and print char by char when we go into
our printing sections.

These next two lines: `call print_string` and `jmp $` are extremely simple,
We first call the memory location for print_string. This is done so we can return to this memory location after printing, and the next line just jumps to itself.
over and over again, so it makes an infinite loop, which makes the BIOS hang until we stop the process.

### The "print_string" section:

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

The first line "`mov ah, 0Eh`" moves the value '`0Eh`' into the '`ah`' register.
In BIOS interrupt services, '`ah`' typically specifies the function being requested. In this case,
'`0Eh`' is the function number for teletype output.

Then we go into the 'print_char' section; this will be called over and over.
again until we print the end of our string

The "`lodsb`" instruction (as said in the comment) loads the byte into memory.
address pointed by the '`si`' register (which in the beginning would be the first character) into the
"`al`" register, and then increments the '`si`' register to point to the next byte in memory.

Next we have "`cmp al, 0`" and "`je printing_finished`"
The first instruction compares the value in the '`al`' register.
with the value '`0`', checking if it's the null terminator, which marks the end of the string.
After that, we use '`je`' (which means jump if equal) instruction to jump to 'printing_finished'
if the value in the '`si`' is the null terminator, which is 0.

Then, simply, if we are not at the end of our string, we carry out our final
instructions: "`int 10h`" and "`jmp print_char`".
The first of the two does the interrupt service '`10h`' which
is the BIOS interrupt for video services, and in this case, the value in '`ah`'
is '`0Eh`' indicating a teletype output, so the byte in '`al`'
is interpreted into a character using ASCII and is then printed to the screen.

Then, in our final section, we return to the beginning to go into our infinite loop.

### Final Two lines

I'll now explain the last two lines, which may look a little confusing. These are:

```x86asm
times 510-($-$$) db 0
dw 0xAA55
```

The first line tells the assembler to add enough zeroes at the end of
the bootloader code to make it exactly 510 bytes long. This ensures that the
Bootloader fills up most of the available space in the 512-byte sector reserved.
for the bootloader.
Then, the next line marks the end of the bootloader. It's like the signing of the
bootloader with a specific code: '`0xAA55`' which tells the BIOS that
This is a valid boot sector. When the BIOS loads the bootloader, it checks for this signature.
to make sure it's a legitimate bootable sector before proceeding with the boot process.

That wraps up our printing, which will be utilised a couple times in the BIOS.
Although I'm pretty sure when we move out of the bios, we will have to print in a different
way, as we will not have access to the bios teletype services.
Now, it's time to move onto our section, which will load our kernel into memory.

Before we move on, here is the makefile for our code:

```makefile
# Compiler
NASM := nasm

# Compiler flags
NASMFLAGS := -f bin

# Source files
SRC := print.asm

build: $(SRC)
    $(NASM) $(NASMFLAGS) -o print.o $(SRC)
    dd if=print.o of=print.img
    qemu-system-x86_64 -s print.img
    rm -f print.o

clean:
    rm -f *.o *.img
```

## Loading the kernel

In order to load the kernel, we actually need a kernel to load. Here's a simple one:

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
    je dones
    
    int 10h

    jmp print_char

done:
    ret

    hello_string db 'Hello World!, i am lytlnyblOS', 0
```

This code is extremely similar to the printing code we wrote earlier, with the only real difference being how x86 segmentation
is handled in the first line of code; instead of loading the segment of the bootloader into the data segment register, we instead
Take the code segment and load it into the data segment, so the data segment is in the same place as the code segment.
is.

Easy enough after focusing that much on printing earlier. Now let's modify the bootloader to accommodate loading.
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
    jmp 0900h:0000 ; gives control to the kernel by jumping to it's starting point.

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

    ; if successful, carry flag will be set to 0, otherwise carry flag is 1
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
But I will explain it all as best as I can now in order of logical operation, and then in later chapters we can cover these x86 concepts.
in the next chapter of this series.

### The "load_kernel_from_disk" section:

Carrying on in order of logical operation, after we print two times (for loading and intro messages), we go straight
into our section for loading the kernel from disc, and the only goal of this is to take the kernel from the device's storage.
and loading it into memory.

To start doing this, we first set up the segment address, which is '`0900h`' which is loaded into
the '`ax`' register, then copied to the es register; this segment address is where the kernel will be loaded.
into memory.

Next, with lines "`mov ah, 02h`" and "`mov al, 01h`" we are setting up our
disc read parameters, the '`ah`' register is set to 02h, indicating the BIOS read functionality.
Then, the '`al`' register is set to 01h, indicating that only one sector will be read from (our kernel is small).
so it only takes up one sector.

The next lines, "`mov ch, 02h`" and "`mov cl, 02h`" require knowledge about
discs rather than segmentation, in the first line, '`ah`' gets 0h loaded into it (which is just 0).
This is the cylinder we want to read from; this would just be the 0th cylinder, as our program is not big enough to be stored.
on multiple cylinders. The '`al`' register stores the segment number we want to read from; this is
the reason our virtual machine would use a floppy disk, as the segments in a floppy disk are 512 bytes, which is the size
of our bootloader, so we can easily just read from the second sector on the disk.

Then next, we specify the disc we are reading from with the lines: "`mov dh, 0h`" and
"`mov dl, 80h`" The first instruction defines the type of disc that we are going to move from.
'`0h`' signifies that we are reading from a floppy disk. The next line is simply the number of discs we are reading from:
'`80h`' signifies that we are reading from disc 0, and 81h would be disc 1.

Then "`mov bx, 0h`" just signifies the offset of the  segment we will load into, which will just be 0
as we want to load it into the start of our segment.

Our final line for reading is just "`int 13h`" which is an instruction that calls
the BIOS interrupt, which performs the disc read operation based on the provided parameters.

Then the only thing left to do is check for errors; the interrupt earlier would set the carry flag if there was an error.
So we can just use jc (jump if carry) to jump to an error handling sub-routine, which will just output a message signifying
an error and then create an infinite loop.

That's all on reading from the disc; let's now look at the changes that were made to printing, which only really
Just allow us to print multiple lines.

### Printing Changes:

The only real changes to printing made in our code are the changes to the printing_finished section of our code, as
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

All this code is pretty basic; in the first block, our goal is to make a new line, and our comments explain that pretty well enough
What we do next is read the current cursor position; this is ultimately not needed, but we can do conditions for this to print error messages if
the cursor row is in the incorrect place, dh will store the row number, so we can jump to an error message if it is not in the correct row.
The final block is to move the cursor to the beginning of the current row after making a new line, which is also explained in the comments for the code.

There we have it. After writing all this, you can say you've made your own operating system (albeit a very simple one).
But it's a bootloader that loads a kernel, which then outputs a message, which may seem pretty dull, but just consider the fact that
that this was all done on bare metal hardware without an OS to support us, which is pretty cool.

Here is the makefile for the kernel and bootloader:

```makefile
BOOT_FILE = bootloader/bootloader.asm 
KERNEL_FILE = kernel/basic_kernel.asm 
        
build: $(BOOT_FILE) $(KERNEL_FILE)
    nasm -f bin $(BOOT_FILE) -o bootstrap.o
    nasm -f bin $(KERNEL_FILE) -o kernel.o
    dd if=bootstrap.o of=kernel.img
    dd seek=1 conv=sync if=kernel.o of=kernel.img bs=512
    qemu-system-x86_64 -s kernel.img
            
clean:
    rm -f *.o
```

