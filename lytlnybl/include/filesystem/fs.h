#ifndef FS_H
#define FS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "filesystem/fs_layout.h"

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
