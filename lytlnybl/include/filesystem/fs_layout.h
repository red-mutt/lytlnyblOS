#ifndef FS_LAYOUT_H
#define FS_LAYOUT_H

#include <stdint.h>

#define FS_BLOCK_SIZE 512

#define FS_START_BLOCK 70
#define FS_SUPERBLOCK 0 // 1 block
#define FS_BITMAP_BLOCK 1 //1 block
#define FS_INODE_START 2 //block count calculated when formatting

#define FS_MAX_INODES 128
#define FS_FILENAME_LENGTH 32

#define FS_TYPE_FREE 0
#define FS_TYPE_FILE 1
#define FS_TYPE_DIRECTORY 2

#define FS_INODE_MAX_BLOCKS 10
#define FS_TOTAL_BLOCKS 1000
#define FS_TOTAL_INODES 128

#define FS_ROOT_INODE 0

#define FS_UINT_ERROR UINT32_MAX

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

  uint32_t blocks[FS_INODE_MAX_BLOCKS];
} fs_inode_t; 

typedef struct {
  uint32_t inode;
  char name[FS_FILENAME_LENGTH];
} fs_directory_entry_t;

#endif
