#ifndef FS_H
#define FS_H

#include <stdint.h>
#include <stdbool.h>

#include "ata.h"

#define FS_BLOCK_SIZE 512

#define FS_START_BLOCK 50
#define FS_SUPERBLOCK FS_START_BLOCK // 1 block
#define FS_BITMAP_BLOCK FS_START_BLOCK + 1 //2 blocks
#define FS_INODE_START FS_START_BLOCK + 3 //10 blocks
#define FS_BLOCKS FS_START_BLOCK + 13

#define FS_MAX_INODES 128
#define FS_FILENAME_LENGTH 32

#define FS_TYPE_FREE 0
#define FS_TYPE_FILE 1
#define FS_TYPE_DIRECTORY 2

#define FS_MAGIC 0xDEADBABE

typedef struct {
  uint32_t magic;

  uint32_t block_size;
  uint32_t total_blocks;

  uint32_t bitmap_start;
  
  uint32_t inode_start;
  uint32_t inode_count;
  uint32_t inode_blocks;
  uint32_t root_inode;

  uint32_t data_start;

  uint32_t free_blocks;
  uint32_t free_inodes;
} fs_superblock_t;

typedef struct {
  uint32_t size;

  uint16_t type;
  uint16_t links;

  uint32_t blocks[10];
} fs_inode_t; 

typedef struct {
  uint32_t inode;
  char name[FS_FILENAME_LENGTH];
} fs_directory_entry_t;

bool fs_read_block(uint32_t block, void* buffer);
bool fs_write_block (uint32_t block, const void* buffer);

bool init_fs(void);


#endif
