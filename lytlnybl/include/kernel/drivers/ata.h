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
