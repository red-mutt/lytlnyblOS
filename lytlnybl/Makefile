CC = i686-elf-gcc
HOST_CC = gcc
LD = ld
NASM = nasm
OBJCOPY = objcopy

CFLAGS = -g -m32 -Iinclude -ffreestanding
USER_CFLAGS = $(CFLAGS) -fno-pie -fno-pic

BUILD_DIR = build

KERNEL_ENTRY_ASM = kernel/kernel_intro.asm
KERNEL_ENTRY_OBJECT = $(BUILD_DIR)/$(KERNEL_ENTRY_ASM:.asm=.asm.o)

KERNEL_C_FILES = $(shell find kernel memory filesystem tasks -name '*.c')
KERNEL_ASM_FILES = $(shell find kernel memory tasks -name '*.asm')

USER_C_FILES = $(shell find user -name '*.c')
USER_ASM_FILES = $(shell find user -name '*.asm')

KERNEL_C_OBJECTS = $(KERNEL_C_FILES:%.c=$(BUILD_DIR)/%.o)
KERNEL_ASM_OBJECTS = $(KERNEL_ASM_FILES:%.asm=$(BUILD_DIR)/%.asm.o)

USER_C_OBJECTS = $(USER_C_FILES:%.c=$(BUILD_DIR)/%.o)
USER_ASM_OBJECTS = $(USER_ASM_FILES:%.asm=$(BUILD_DIR)/%.asm.o)

BOOTLOADER = $(BUILD_DIR)/bootloader.bin

USER_ELF = $(BUILD_DIR)/user_test.elf
USER_BIN = $(BUILD_DIR)/user_test.bin
USER_BINARY_O = $(BUILD_DIR)/user_test_binary.o

KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
KERNEL_IMG = $(BUILD_DIR)/kernel.img

USER_LINKER = user/user.ld
KERNEL_LINKER = kernel/linker.ld

ROOTFS_DIR = rootfs
ROOTFS_BIN_DIR = $(ROOTFS_DIR)/bin

USER_ROOTFS_BIN = $(ROOTFS_BIN_DIR)/user_test

MKFS = $(BUILD_DIR)/mkfs




.PHONY: build clean

build: $(KERNEL_IMG) $(USER_ROOTFS_BIN)


$(MKFS): tools/mkfs.c
	mkdir -p $(dir $@)
	$(HOST_CC) -g -Iinclude $< -o $@

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.asm.o: %.asm
	mkdir -p $(dir $@)
	$(NASM) -f elf32 -g -F dwarf $< -o $@

$(BOOTLOADER): boot/bootloader.asm
	mkdir -p $(dir $@)
	$(NASM) -f bin $< -o $@

$(USER_ELF): $(USER_C_OBJECTS) $(USER_ASM_OBJECTS)
	mkdir -p $(dir $@)
	$(LD) -m elf_i386 -T $(USER_LINKER) \
		$(USER_C_OBJECTS) \
		$(USER_ASM_OBJECTS) \
		-o $@

$(USER_BIN): $(USER_ELF)
	$(OBJCOPY) -O binary $< $@

$(USER_ROOTFS_BIN): $(USER_BIN)
	mkdir -p $(dir $@)
	cp $< $@

$(USER_BINARY_O): $(USER_BIN)
	cd $(BUILD_DIR) && $(OBJCOPY) -I binary -O elf32-i386 -B i386 user_test.bin user_test_binary.o

$(KERNEL_ELF): $(KERNEL_C_OBJECTS) $(KERNEL_ASM_OBJECTS) $(USER_BINARY_O)
	mkdir -p $(dir $@)
	$(LD) -m elf_i386 -T $(KERNEL_LINKER) \
		$(KERNEL_ENTRY_OBJECT) \
		$(KERNEL_C_OBJECTS) \
		$(filter-out $(KERNEL_ENTRY_OBJECT),$(KERNEL_ASM_OBJECTS)) \
		$(USER_BINARY_O) \
		-o $@

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

$(KERNEL_IMG): $(BOOTLOADER) $(KERNEL_BIN) $(MKFS)
	dd if=/dev/zero of=$@ bs=512 count=20480
	dd if=$(BOOTLOADER) of=$@ conv=notrunc
	dd if=$(KERNEL_BIN) of=$@ seek=1 conv=notrunc
	$(MKFS) $@

run: $(KERNEL_IMG)
	qemu-system-i386 -drive format=raw,file=$(KERNEL_IMG) #-s -S

clean:
	rm -rf $(BUILD_DIR)
	
