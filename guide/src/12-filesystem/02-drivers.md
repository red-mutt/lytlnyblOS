# Writing the Driver

## Expanding the Disk Image

Before we talk about any context, or start writing any code, I just want to make sure the filesystem is large
enough to write to the file system without overwriting the kernel, doing this will also make it so we don't 
have to expand the sectors read by the bootloader every time our kernel increases 512 bytes in size.

I will show you Linux shell commands and not the Makefile as you may want to customize your Makefile
differently. Currently, your Makefile may contain commands like this:

```makefile
ld -m elf_i386 -T kernel/linker.ld kernel.o kernel_main.o vga.o interruptc.o interrupta.o timer.o kb.o pmm.o... -o kernel.elf

objcopy -O binary kernel.elf kernel.bin
dd if=bootstrap.o of=kernel.img
dd if=kernel.bin of=kernel.img seek=1 conv=notrunc
qemu-system-i386 -drive format=raw,file=kernel.img 
```

The most important commands here are the last three, this covers the creation
of `kernel.img` (which is our disk image). In the first of these three
`bootstrap.o` copies to `kernel.img` at block 0.
In the next one, the `kernel.bin` is then written to `kernel.img` at
block 1, then `conv=notrunc` tells `dd` to not truncate the existing
kernel.img when writing the kernel. The resulting `kernel.img` is 
then only big enough to contain the bootloader and kernel.

We need an extra command to extend kernel.img with extra space for 
our file system. Our new set of commands should be:

```makefile
dd if=/dev/zero of=kernel.img bs=512 count=20480
dd if=bootstrap.o of=kernel.img conv=notrunc
dd if=kernel.bin of=kernel.img seek=1 conv=notrunc
```

The first command creates a 10Mib (512 bytes x 20480) file called `kernel.img` filled with zeros.
It does this using `/dev/zero` which is a special device on Linux that produces
an endless stream of zero bytes. Now you can write to the disk without worrying about size.

## Context on the driver

As I said in the file system, we are going to use **ATA PIO** for the driver.
ATA is the interface used to communicate with the disk, while PIO (Programmed I/O)
is the method that we will be using to transfer the data. This is an old but 
simple way of accessing an ATA disk, but it's useful for us because it allows us 
to communicate with the disk using a few I/O ports and not needing to 
write a complicated storage driver.

The goal here, like with all our operating system's infrastructure, is to
keep the driver simple and short. We only need two main operations for our file 
system: **reading one sector** and **writing one sector**. We identify which sector
using the LBA (Logical Block Address), and each sector will contain 
512 bytes.

The LBA (despite the name) is the way we identify a particular sector on a disk
using a single number. Instead of thinking about the disk as having physical co-ordinates
like a head that leads to a cylinder that leads to a sector. The LBA simply just allows
us to treat the disk like a long sequence of sectors. Wanting to read LBA 31 will just
read the sector numbered 31.

I also said prior that another goal with this is to hide all ATA-specific details
from the rest of the operating system. This is important because the file system
shouldn't need to know something like which I/O ports are used 
or which commands are required to read a sector.

Many ATA features won't get created. We will use the primary ATA channel,
the master drive, 28-bit LBA addressing, and PIO transfers. This is enough
for our simple file system and keeps the driver easy to understand.

## The implementation

The header:

```c
#ifndef ATA_H
#define ATA_H

#include <stdint.h>
#include <stdbool.h>

#define ATA_DATA 0x1F0
#define ATA_SECTOR_COUNT 0x1F2
#define ATA_LBA_LOW 0x1F3
#define ATA_LBA_MID 0x1F4
#define ATA_LBA_HIGH 0x1F5
#define ATA_DRIVE 0x1F6
#define ATA_STATUS 0x1F7
#define ATA_COMMAND 0x1F7

#define ATA_STATUS_ERR 0x01
#define ATA_STATUS_DRQ 0x08
#define ATA_STATUS_DF 0x20
#define ATA_STATUS_BSY 0x80

#define ATA_CMD_READ_PIO 0x20
#define ATA_CMD_WRITE_PIO 0x30
#define ATA_CMD_FLUSH_CACHE 0xE7

#define ATA_DRIVE_MASTER 0xE0

#define ATA_SECTOR_SIZE 512

#define ATA_ALT_STATUS 0x3F6

void init_ata(void);

bool ata_read_sector(uint32_t lba, void* buffer);
bool ata_write_sector(uint32_t lba, const void* buffer);

#endif
```

The first group of definitions contains the I/O ports used to communicate with
the ATA controller. THese numbers come from the standard layout of the primary
ATA channel. `ATA_STATUS`and `ATA_COMMAND` have the same value. This is intentional. The register at this 
port is used for different purposes depending on whether we are reading from it or writing to it:
reading gives us the status register, while writing sends a command to the controller.

The second group represents individual bits in the ATA status
register. We can use them with bit-wise operations to check the state of the controller.
For example: `status & ATA_STATUS_BSY` checks whether the `BSY bit is set.
If it's set, the ATA controller is currently busy.

The next set are the commands we will send to the ATA controller. 
`0x20` tells it to perform a PIO read, while `0x30` tells it to perform a
PIO write. The third is the command for flushing cache. After writing a sector,
this command is sent, and then we wait for the controller to finish. This makes
sure data has been flushed from the drive's cache before we report that the write has
completed.

`ATA_DRIVE_MASTER` contains the bits we need when selecting the master drive and
using LBA addressing. We will use this value when selecting our disk before performing
a read or write.

### Changes to Interrupts

Before we look at the implementation file, there are some changes we will
have to make to our interrupts code. Our interrupts code is where we made
the assembly labels for `inb` and `outb`. Because this is a driver, the code
for the ATA will also require these functions.

The ATA will also require new labels too, these being `inw` and `outw` for retrieving and
sending words (2 bytes) instead of bytes to I/O ports.

Here are additions for `interrupts.h`:

```c
extern void outw(uint16_t port, uint16_t value);
extern uint16_t inw(uint16_t port);
```

This is a required addition for `interrupts.asm`:

```x86asm
global outw
global inw

outw:
    mov dx, [esp + 4]
    mov ax, [esp + 8]
    out dx, ax
    ret
inw:
    mov dx, [esp + 4]
    in ax, dx
    movzx eax, ax
    ret
```

Finally, here is `ata.c`:

```c
#include "kernel/drivers/ata.h"
#include "kernel/interrupts.h"


static void ata_400ns_delay() {
  inb(ATA_ALT_STATUS);
  inb(ATA_ALT_STATUS);
  inb(ATA_ALT_STATUS);
  inb(ATA_ALT_STATUS);
}

static void ata_wait_bsy() {
  uint8_t status;
  do {
    status = inb(ATA_STATUS);
  } while(status & ATA_STATUS_BSY);
}

static bool ata_wait_drq() {
  uint8_t status; 

  for(;;) {
    status = inb(ATA_STATUS);
    if (status & ATA_STATUS_BSY) continue;
    if (status & ATA_STATUS_ERR) return false;
    if (status & ATA_STATUS_DF) return false;
    if (status & ATA_STATUS_DRQ) return true;
  }
}

static void ata_select_drive(uint32_t lba) {
  outb(ATA_DRIVE, ATA_DRIVE_MASTER | ((lba >> 24) & 0x0F));
  ata_400ns_delay();
  ata_wait_bsy();
}

void init_ata() {
  outb(ATA_DRIVE, ATA_DRIVE_MASTER);
  ata_400ns_delay();
}

bool ata_read_sector(uint32_t lba, void* buffer) {
  uint16_t *data = (uint16_t*)buffer;

  if (lba > 0x0FFFFFFF) return false;

  ata_select_drive(lba);

  outb(ATA_SECTOR_COUNT, 1);

  outb(ATA_LBA_LOW, lba & 0xFF);
  outb(ATA_LBA_MID, (lba >> 8) & 0xFF);
  outb(ATA_LBA_HIGH, (lba >> 16) & 0xFF);

  outb(ATA_COMMAND, ATA_CMD_READ_PIO);

  if (!ata_wait_drq()) return false;

  for (int i = 0; i < 256; i++) {
    data[i] = inw(ATA_DATA);
  }

  ata_400ns_delay();

  return true;
}

bool ata_write_sector(uint32_t lba, const void* buffer) {
  const uint16_t *data = (const uint16_t*)buffer;

  if (lba > 0x0FFFFFFF) return false;

  ata_select_drive (lba);

  outb(ATA_SECTOR_COUNT, 1);

  outb(ATA_LBA_LOW, lba & 0xFF);
  outb(ATA_LBA_MID, (lba >> 8) & 0xFF);
  outb(ATA_LBA_HIGH, (lba >> 16) & 0xFF);

  outb(ATA_COMMAND, ATA_CMD_WRITE_PIO);

  if (!ata_wait_drq()) return false;

  for (int i = 0; i < 256; i++) {
    outw(ATA_DATA, data[i]);
  }

  ata_400ns_delay();
  outb(ATA_COMMAND, ATA_CMD_FLUSH_CACHE);
  ata_wait_bsy();

  return true;
}
```

### `ata_400ns_delay()`

```c
static void ata_400ns_delay() {
  inb(ATA_ALT_STATUS);
  inb(ATA_ALT_STATUS);
  inb(ATA_ALT_STATUS);
  inb(ATA_ALT_STATUS);
}
```

This function may look weird at first glance, but it covers an ATA-specific
detail. Reading the alternate status port four times provides a required
delay for the traditional ATA interface. This delay is roughly 400ns on 
traditional ATA interface. ATA is much slower than the CPU; after commands like 
drive selection, the device will need a small amount of time to process the
write. The CPU could otherwise execute the next instruction almost immediately before
the drive has actually been selected.

### `ata_wait_bsy`

```c
static void ata_wait_bsy() {
  uint8_t status;
  do {
    status = inb(ATA_STATUS);
  } while(status & ATA_STATUS_BSY);
}
```

This function does what is referred to as **polling**. Polling is the process
where a computer program repeatedly checks the status of another device or resource
at regular intervals to see if it needs attention or has data ready.
The function does this by repeatedly reading the status register and checks
`status & ATA_STATUS_BSY`. As long as BSY is set, the function keeps waiting.
When BSY becomes clear, the device is no longer busy and the function returns.

### `ata_wait_drq()`

```c
static bool ata_wait_drq() {
  uint8_t status; 

  for(;;) {
    status = inb(ATA_STATUS);
    if (status & ATA_STATUS_BSY) continue;
    if (status & ATA_STATUS_ERR) return false;
    if (status & ATA_STATUS_DF) return false;
    if (status & ATA_STATUS_DRQ) return true;
  }
}
```

After sending a read or write command, the device doesn't necessarily become
ready immediately. The driver waits until 

-   `ERR` gets set: An ATA error occurred.
-   `DF` gets set: A device fault occurred.
-   `DRQ` gets set: The device is ready for a data transfer.

The most important of these is `DRQ`. For a read, it means the device
has data ready for the CPU to retrieve. For a write, it means the device
is ready to receive data from the CPU.

### `ata_select_drive()`

```c
static void ata_select_drive(uint32_t lba) {
  outb(ATA_DRIVE, ATA_DRIVE_MASTER | ((lba >> 24) & 0x0F));
  ata_400ns_delay();
  ata_wait_bsy();
}
```




