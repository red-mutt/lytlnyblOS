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

KERNEL_C_OBJECTS = $(KERNEL_C_FILES:%.c=$(BUILD_DIR)/%.o)
KERNEL_ASM_OBJECTS = $(KERNEL_ASM_FILES:%.asm=$(BUILD_DIR)/%.asm.o)

# USER STUFFS

USER_C_OBJECTS = $(USER_C_FILES:%.c=$(BUILD_DIR)/%.o)
USER_ASM_OBJECTS = $(USER_ASM_FILES:%.asm=$(BUILD_DIR)/%.asm.o)

#need to build an archive for nice linking and file sizes
AR = i686-elf-ar

USER_LIB = $(BUILD_DIR)/libc.a

USER_LIB_C_FILES = $(shell find user/libc -name '*.c')
USER_LIB_C_OBJECTS = $(USER_LIB_C_FILES:%.c=$(BUILD_DIR)/%.o)

USER_LIB_ASM_FILES = $(shell find user/libc -name '*.asm')
USER_LIB_ASM_OBJECTS = $(USER_LIB_ASM_FILES:%.asm=$(BUILD_DIR)/%.asm.o)

USER_C_FILES = $(shell find user -name '*.c')
USER_ASM_FILES = $(shell find user -name '*.asm')

# ADD TO THIS WHEN ADDING PROGRAMS

USER_TEST_ELF = $(BUILD_DIR)/user_test.elf
USER_TEST_BIN = $(BUILD_DIR)/user_test.bin

SHELL_ELF = $(BUILD_DIR)/shell.elf
SHELL_BIN = $(BUILD_DIR)/shell.bin

USER_TEST_ROOTFS = $(ROOTFS_BIN_DIR)/user_test
SHELL_ROOTFS = $(ROOTFS_BIN_DIR)/shell

USER_BINARY_O = $(BUILD_DIR)/programs/test_binary.o

#END OF USER STUFF

BOOTLOADER = $(BUILD_DIR)/bootloader.bin


KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
KERNEL_IMG = $(BUILD_DIR)/kernel.img

USER_LINKER = user/user.ld
KERNEL_LINKER = kernel/linker.ld

ROOTFS_DIR = rootfs
ROOTFS_BIN_DIR = $(ROOTFS_DIR)/bin

USER_ROOTFS_BIN = $(ROOTFS_BIN_DIR)/user_test $(ROOTFS_BIN_DIR)/shell

MKFS = $(BUILD_DIR)/mkfs




.PHONY: build clean



# ADD TO BUILD RULES WHEN ADDING USER PROGRAMS
build: $(KERNEL_IMG) $(USER_TEST_ROOTFS) $(SHELL_ROOTFS)

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

# ------------------
# MAIN USER COMPILATION AND COPYING STUFF
# ADD NEW RULES FOR NEW PROGRAMS

$(USER_LIB): $(USER_LIB_C_OBJECTS) $(USER_LIB_ASM_OBJECTS)
	mkdir -p $(dir $@)
	$(AR) rcs $@ $^

$(USER_TEST_ELF): $(BUILD_DIR)/user/programs/test.o $(USER_LIB)
	mkdir -p $(dir $@)
	$(LD) -m elf_i386 -T $(USER_LINKER) $^ -o $@

$(USER_TEST_BIN): $(USER_TEST_ELF)
	$(OBJCOPY) -O binary $< $@

$(SHELL_ELF): $(BUILD_DIR)/user/programs/shell.o  $(USER_LIB)
	mkdir -p $(dir $@)
	$(LD) -m elf_i386 -T $(USER_LINKER) $^ -o $@

$(SHELL_BIN): $(SHELL_ELF)
	$(OBJCOPY) -O binary $< $@

# ------------------
# REMEMBER TO COPY TO ROOTFS TOO

$(USER_TEST_ROOTFS): $(USER_TEST_BIN)
	mkdir -p $(dir $@)
	cp $< $@

$(SHELL_ROOTFS): $(SHELL_BIN)
	mkdir -p $(dir $@)
	cp $< $@

# ------------------------

$(KERNEL_ELF): $(KERNEL_C_OBJECTS) $(KERNEL_ASM_OBJECTS) 
	mkdir -p $(dir $@)
	$(LD) -m elf_i386 -T $(KERNEL_LINKER) \
		$(KERNEL_ENTRY_OBJECT) \
		$(KERNEL_C_OBJECTS) \
		$(filter-out $(KERNEL_ENTRY_OBJECT),$(KERNEL_ASM_OBJECTS)) \
		-o $@

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

# ADD HERE TOO, the ROOTFS
$(KERNEL_IMG): $(BOOTLOADER) $(KERNEL_BIN) $(MKFS) $(USER_TEST_ROOTFS) $(SHELL_ROOTFS)
	dd if=/dev/zero of=$@ bs=512 count=20480
	dd if=$(BOOTLOADER) of=$@ conv=notrunc
	dd if=$(KERNEL_BIN) of=$@ seek=1 conv=notrunc
	$(MKFS) $@

run: $(KERNEL_IMG)
	qemu-system-i386 -drive format=raw,file=$(KERNEL_IMG) #-s -S

clean:
	rm -rf $(BUILD_DIR)
	
