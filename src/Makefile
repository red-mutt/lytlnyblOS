BOOT_FILE = bootloader/bootloader.asm 
KERNEL_FILE = kernel/basic_kernel.asm 
KERNEL_FILES_C = kernel/kernel_main.c
VGA_FILE_C = kernel/vga_text.c
INTERRUPT_FILE_C = kernel/interrupts.c
INTERRUPT_FILE_ASM = kernel/interrupts.asm
TIMER_FILE_C = kernel/timer.c
kb_file_c = kernel/keyboard.c
PMM_FILE_C = memory/pmm.c
VMM_FILE_C = memory/vmm.c
VMM_FILE_ASM = memory/vmm.asm
HEAP_FILE_C = memory/heap.c
TASKS_FILE_C = tasks/procman.c
TASKS_FILE_ASM = tasks/procman.asm
CONTEXT_FILE_C = tasks/context.c
SCHEDULER_FILE_C = tasks/scheduler.c
ATA_FILE_C = filesystem/ata.c
FS_FILE_C = filesystem/fs.c
FS_M_FILE_C = filesystem/fs_manager.c
KERNEL_UTILS_C = kernel/kernel_utils.c


USER_TEST_FILE_C = user/programs/test.c
SYSCALLS_ASM = user/libc/syscalls.asm
SYSCALLS_C = user/libc/syscalls.c
OUTPUT_C = user/libc/output.c
MEMORY_C = user/libc/memory.c
USER_LINKER = user/programs/user.ld

LINKER = kernel/linker.ld

CC = i686-elf-gcc
        
build: $(BOOT_FILE) $(KERNEL_FILE)
	nasm -f bin $(BOOT_FILE) -o bootstrap.o
	nasm -f elf32 -g -F dwarf $(KERNEL_FILE) -o kernel.o
	$(CC) -g -m32 -ffreestanding -c $(KERNEL_FILES_C) -o kernel_main.o
	$(CC) -g -m32 -ffreestanding -c $(VGA_FILE_C) -o vga.o
	$(CC) -g -m32 -ffreestanding -c $(INTERRUPT_FILE_C) -o interruptc.o
	nasm -f elf32 -g -F dwarf $(INTERRUPT_FILE_ASM) -o interrupta.o
	$(CC) -g -m32 -ffreestanding -c $(TIMER_FILE_C) -o timer.o
	$(CC) -g -m32 -ffreestanding -c $(kb_file_c) -o kb.o
	$(CC) -g -m32 -ffreestanding -c $(PMM_FILE_C) -o pmm.o
	$(CC) -g -m32 -ffreestanding -c $(VMM_FILE_C) -o vmm.o
	nasm -f elf32 -g -F dwarf $(VMM_FILE_ASM) -o vmma.o
	$(CC) -g -m32 -ffreestanding -c $(HEAP_FILE_C) -o heap.o
	$(CC) -g -m32 -ffreestanding -c $(TASKS_FILE_C) -o tasks.o
	$(CC) -g -m32 -ffreestanding -c $(CONTEXT_FILE_C) -o context.o
	$(CC) -g -m32 -ffreestanding -c $(SCHEDULER_FILE_C) -o scheduler.o
	nasm -f elf32 -g -F dwarf $(TASKS_FILE_ASM) -o tasksa.o
	$(CC) -g -m32 -ffreestanding -c $(ATA_FILE_C) -o ata.o
	$(CC) -g -m32 -ffreestanding -c $(FS_FILE_C) -o fs.o
	$(CC) -g -m32 -ffreestanding -c $(FS_M_FILE_C) -o fsm.o
	$(CC) -g -m32 -ffreestanding -c $(KERNEL_UTILS_C) -o kernel_utils.o

	#user tests
	$(CC) -g -m32 -ffreestanding -fno-pie -fno-pic -c $(USER_TEST_FILE_C) -o user_test.o
	nasm -f elf32 -g -F dwarf $(SYSCALLS_ASM) -o syscalls_a.o
	$(CC) -g -m32 -ffreestanding -fno-pie -fno-pic -c $(SYSCALLS_C) -o syscalls_c.o
	$(CC) -g -m32 -ffreestanding -fno-pie -fno-pic -c $(OUTPUT_C) -o output_c.o
	$(CC) -g -m32 -ffreestanding -fno-pie -fno-pic -c $(MEMORY_C) -o memory_c.o



	ld -m elf_i386 -T $(USER_LINKER) user_test.o syscalls_a.o syscalls_c.o output_c.o memory_c.o -o user_test.elf
	objcopy -O binary user_test.elf user_test.bin
	objcopy -I binary -O elf32-i386 -B i386 user_test.bin user_test_binary.o



	ld -m elf_i386 -T $(LINKER) kernel.o kernel_main.o vga.o interruptc.o interrupta.o timer.o kb.o pmm.o vmm.o vmma.o heap.o tasks.o context.o scheduler.o tasksa.o user_test_binary.o ata.o fs.o fsm.o kernel_utils.o -o kernel.elf

	objcopy -O binary kernel.elf kernel.bin
	dd if=/dev/zero of=kernel.img bs=512 count=20480
	dd if=bootstrap.o of=kernel.img conv=notrunc
	dd if=kernel.bin of=kernel.img seek=1 conv=notrunc
	qemu-system-i386 -drive format=raw,file=kernel.img #-s -S

clean:
	rm -f *.o
	rm -f *.bin
	rm -f *.elf
	rm -f *.lst
	rm -f *.debug
	
