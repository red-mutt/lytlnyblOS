## How do we enter C?

When compiling C code, we can not use the compiler on our current
system, we must make a cross-compiler. I will not be instructing you on
how to do this as it varies a lot based on the OS you are on. I will give
a fair warning that when building the `gcc` cross-compiler you can fail to
compile due to the version of `gcc-c++` you are building with being too
new, and you may need an older version. Here's a resource on setting up
a cross-compiler [here.](wiki.osdev.org/GCC_Cross-Compiler)

Once we have our cross compiler we can simply have this as our
`kernel_main.c` file:


```c
void kernel_main(void)
{
    volatile char* vga = (volatile char*)0xB8000;
    
    //signal that we have reached C
    vga[0] = 'C';
    vga[1] = 0x02;

    for (;;);
}
```

This simply just Writes a green character to our VGA output we must then
link the compilation of this in with our Makefile using our cross-compiler:

```makefile
BOOT_FILE = bootloader/bootloader.asm 
KERNEL_FILE = kernel/basic_kernel.asm 
KERNEL_FILES_C = kernel/kernel_main.c
LINKER = kernel/linker.ld

CC = i686-elf-gcc
        
build: $(BOOT_FILE) $(KERNEL_FILE)
    nasm -f bin $(BOOT_FILE) -o bootstrap.o
    nasm -f elf32 -g -F dwarf $(KERNEL_FILE) -o kernel.o
    $(CC) -m32 -ffreestanding -c $(KERNEL_FILES_C) -o kernel_main.o

    ld -m elf_i386 -T $(LINKER) kernel.o kernel_main.o -o kernel.elf

    objcopy -O binary kernel.elf kernel.bin
    dd if=bootstrap.o of=kernel.img
    dd if=kernel.bin of=kernel.img seek=1 conv=notrunc
    qemu-system-i386 -drive format=raw,file=kernel.img 
            
clean:
    rm -f *.o
```

And then finally within our assembly code we must add:

```x86asm
p_mode_main:
    mov ax, 10h
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x9000

    ; go into C

    extern kernel_main
    call kernel_main
```

And then if everything's done correctly we will be in C signified by
the first character on our screen being replaced by a green C

