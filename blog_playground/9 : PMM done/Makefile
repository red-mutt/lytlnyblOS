BOOT_FILE = bootloader/bootloader.asm 
KERNEL_FILE = kernel/basic_kernel.asm 
KERNEL_FILES_C = kernel/kernel_main.c
VGA_FILE_C = kernel/vga_text.c
INTERRUPT_FILE_C = kernel/interrupts.c
INTERRUPT_FILE_ASM = kernel/interrupts.asm
TIMER_FILE_C = kernel/timer.c
kb_file_c = kernel/keyboard.c
PMM_FILE_C = memory/pmm.c
LINKER = kernel/linker.ld

CC = i686-elf-gcc
        
build: $(BOOT_FILE) $(KERNEL_FILE)
	nasm -f bin $(BOOT_FILE) -o bootstrap.o
	nasm -f elf32 -g -F dwarf $(KERNEL_FILE) -o kernel.o
	$(CC) -m32 -ffreestanding -c $(KERNEL_FILES_C) -o kernel_main.o
	$(CC) -m32 -ffreestanding -c $(VGA_FILE_C) -o vga.o
	$(CC) -m32 -ffreestanding -c $(INTERRUPT_FILE_C) -o interruptc.o
	nasm -f elf32 -g -F dwarf $(INTERRUPT_FILE_ASM) -o interrupta.o
	$(CC) -m32 -ffreestanding -c $(TIMER_FILE_C) -o timer.o
	$(CC) -m32 -ffreestanding -c $(kb_file_c) -o kb.o
	$(CC) -m32 -ffreestanding -c $(PMM_FILE_C) -o pmm.o

	ld -m elf_i386 -T $(LINKER) kernel.o kernel_main.o vga.o interruptc.o interrupta.o timer.o kb.o pmm.o -o kernel.elf

	objcopy -O binary kernel.elf kernel.bin
	dd if=bootstrap.o of=kernel.img
	dd if=kernel.bin of=kernel.img seek=1 conv=notrunc
	qemu-system-i386 -drive format=raw,file=kernel.img 
            
clean:
	rm -f *.o

