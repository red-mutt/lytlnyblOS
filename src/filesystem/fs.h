#ifndef FS_H
#define FS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "ata.h"

#define FS_BLOCK_SIZE 512

#define FS_START_BLOCK 70
#define FS_SUPERBLOCK FS_START_BLOCK // 1 block
#define FS_BITMAP_BLOCK FS_START_BLOCK + 1 //1 block
#define FS_INODE_START FS_START_BLOCK + 2 //block count calculated when formatting

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

bool fs_read_block(uint32_t block_num, void* buffer);
bool fs_write_block (uint32_t block_num, const void* buffer);

bool fs_write_superblock(const fs_superblock_t* superblock);
bool fs_read_superblock(fs_superblock_t* superblock);

bool fs_format(void);

int32_t fs_alloc_block(void);
bool fs_free_block(uint32_t block_num);

bool fs_read_inode(uint32_t inode_num, fs_inode_t* inode);
bool fs_write_inode(uint32_t inode_num, const fs_inode_t* inode);

int32_t fs_alloc_inode(uint8_t type);
bool fs_free_inode(uint32_t inode_num);

int32_t fs_find_directory_entry(uint32_t directory_inode_num, const char* name);
bool fs_add_directory_entry(uint32_t directory_inode_num, uint32_t inode_number, const char* name);
bool fs_remove_directory_entry(uint32_t directory_inode_num, const char* name);

int32_t fs_create_file(uint32_t directory_inode_num, const char* name);
int32_t fs_create_directory(uint32_t parent_inode_num, const char* name);
int32_t fs_read_file(uint32_t file_inode_num, void* read_buffer, uint32_t size, uint32_t offset);
int32_t fs_write_file(uint32_t inode_num, const void* write_buffer, uint32_t size, uint32_t offset);
bool fs_delete_file(uint32_t directory_inode_num, const char* name);



#endif
