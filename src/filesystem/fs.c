#include "fs.h"

bool fs_read_block(uint32_t block, void* buffer) {
  uint32_t sector = FS_START_BLOCK + block;
  return ata_read_sector(sector, buffer);
}

bool fs_write_block(uint32_t block, const void* buffer) {
  uint32_t sector = FS_START_BLOCK + block;
  return ata_write_sector(sector, buffer);
}

bool fs_write_superblock(const fs_superblock_t* superblock) {
  uint8_t buffer[FS_BLOCK_SIZE];
  uint16_t i;

  for (i = 0; i < FS_BLOCK_SIZE; i++) buffer[i] = 0x00;

  for (i = 0; i < sizeof(fs_superblock_t); i++) 
    buffer[i] = ((uint8_t*)(superblock))[i];

  return fs_write_block(FS_SUPERBLOCK, (void*)buffer);
}

bool fs_read_superblock(fs_superblock_t* superblock) {
  uint8_t buffer[FS_BLOCK_SIZE];
  uint16_t i;

  if (!fs_read_block(FS_SUPERBLOCK, (void*)buffer))
    return false;

  for(i = 0; i < sizeof(fs_superblock_t); i++)
    ((uint8_t*)superblock)[i] = buffer[i];

  if (superblock->magic != FS_MAGIC)
    return false;

  return true;
}

bool fs_format() {
  fs_superblock_t superblock;
  uint8_t buffer[FS_BLOCK_SIZE];

  superblock.magic = FS_MAGIC;
  superblock.block_size = FS_BLOCK_SIZE;

  superblock.total_blocks = 1000;
  superblock.bitmap_start = FS_BITMAP_BLOCK;
  
  superblock.inode_start = FS_INODE_START;
  superblock.inode_count = 128;
  superblock.inode_blocks = FS_BLOCKS - FS_INODE_START;
  superblock.root_inode = 0;

  superblock.data_start = FS_BLOCKS;

  //free inodes and free blocks set later
  
  fs_write_superblock(&superblock);


}

bool init_fs() {
  init_ata();
  return true;
}
