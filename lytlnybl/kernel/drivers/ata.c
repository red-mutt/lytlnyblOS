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

