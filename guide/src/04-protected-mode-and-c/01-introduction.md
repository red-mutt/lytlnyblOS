# Part IV : Protected Mode and C

## A reminder of what we have now

First after we covered so much theory (and honestly it's been a long
time since I've written this guide) we should take a quick look again
at everything we have written. Part two ended us with a Makefile that
had this content:

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

We then have a `bootloader.asm` file, which we will not really have to
change unless something breaks:

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

And then we also have a basic kernel as follows:

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
    lodsb ; sets al = [DS:SI++]

    cmp al, 0
    je done
    
    int 10h

    jmp print_char

done:
    ret

hello_string db 'Hello World!, i am lytlnyblOS, running in real mode', 0
```

As discussed in the previous part where we covered all our content
needed to get out operating system into protected mode. 
You may notice that currently we are still relying on
BIOS interrupts. These BIOS interrupts are actually pretty powerful, and
you can use them to do many things (like write video games that run
within the BIOS) I have done so with the game snake. Linked
[here.](https://github.com/1x31744/Snake-THE-PREQUEL)

## Debugging is key moving forward

With low level programming tasks such as this, it's important that
we have a clean way to debug our programs, despite debugging still being
important in regular programming, it's often omitted and not really
learned to a degree that it should be by most people learning
programming. This is why in this guide I will be intentionally making us
have an error called a **triple fault**, and then we will be using a
debugger to fix it.

You may be asking *\"what is a triple fault?\"* A triple fault is an x86
CPU reset that occurs when the processor encounters an exception, fails
to invoke the exception handler (causing a double fault), and then also
fails to invoke the double fault handler. At that point, the CPU resets
itself. In our QEMU emulator this would look like a bunch of text
flashing on the screen, this is because the system is continually
rebooting itself over and over again.

The debugger we are going to be using is GDB, make sure to install it
before continuing, or install whatever debugger you prefer.

With the compiled state of our bootloader and kernel as of now, using
a debugger will be pretty tricky, this is because within our debugger we
will not be able to access function names, labels, source lines and
variable names (among many other things). We can still use the debugger
like this, but it will function more as a CPU monitor than a source
debugger so it\'s important to configure our build environment so we get
a lot more context when debugging.

### Setting up GDB

Binary files (which is what we are compiling to now) cannot give
functions names, labels and such so the method I am using to get access
to them is I am going to be compiling to the .elf format, I will then be
copying the .elf compilation back into .bin this is because if we use
the .elf file we would have to refactor some of the code in our
bootloader.

To compile to elf must make a linker script. This tells the linker where
to place things in memory. A linker is a program that combines object
files into a final executable and fixes up all the addresses.

My linker script, called `linker.ld` looks like this.
```
ENTRY(start)

SECTIONS
{
    . = 0x9000;

    .text :
    {
        *(.text)
    }

    .data :
    {
        *(.data)
    }

    .bss :
    {
        *(.bss)
    }
}
```

And then we must add two lines to our Makefile, one to link the object
file into an elf, and one to copy the elf into a bin file, we must also
edit another line to compile our kernel into an object file in
the elf format. The bootloader is a plain binary as we will
not be debugging it at the current moment and will only be changing it
to add blocks. This is our new Makefile:

```makefile
BOOT_FILE = bootloader/bootloader.asm 
KERNEL_FILE = kernel/basic_kernel.asm 
LINKER = kernel/linker.ld
        
build: $(BOOT_FILE) $(KERNEL_FILE)
    nasm -f bin $(BOOT_FILE) -o bootstrap.o
    nasm -f elf32 -g -F dwarf $(KERNEL_FILE) -o kernel.o
    ld -m elf_i386 -T $(LINKER) kernel.o -o kernel.elf
    objcopy -O binary kernel.elf kernel.bin
    dd if=bootstrap.o of=kernel.img
    dd if=kernel.bin of=kernel.img seek=1 conv=notrunc
    qemu-system-i386 -drive format=raw,file=kernel.img -s -S
            
clean:
    rm -f *.o
```

The -s flag in QEMU starts a TCP port in 1234 and -S tells QEMU to
freeze at startup, both of these allow us to connect GDB.

