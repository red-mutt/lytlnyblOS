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
  fs_inode_t root_inode;
  uint8_t buffer[FS_BLOCK_SIZE];
  uint16_t i;

  superblock.magic = FS_MAGIC;
  superblock.block_size = FS_BLOCK_SIZE;

  superblock.total_blocks = 1000;
  superblock.bitmap_start = FS_BITMAP_BLOCK;
  
  superblock.inode_start = FS_INODE_START;
  superblock.inode_count = 128;
  superblock.inode_blocks = (superblock.inode_count * sizeof(fs_inode_t) + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
  superblock.root_inode = FS_ROOT_INODE;

  superblock.data_start = superblock.inode_start + superblock.inode_blocks;

  superblock.free_blocks = superblock.total_blocks - superblock.data_start - 1;
  superblock.free_inodes = superblock.inode_count - 1;


  
  if (!fs_write_superblock(&superblock))
    return false;

  for (i = 0; i < FS_BLOCK_SIZE; i++) 
    buffer[i] = 0;

  //mark everything other than the data as used (in the bitmap)
  for (uint32_t block = 0; block < superblock.data_start; block++) {
    uint32_t byte = block / 8;
    uint32_t bit = block % 8;
    buffer[byte] |= (1 << bit);
  }

  //mark root as used in bitmap
  uint32_t root_block = superblock.data_start;
  uint32_t root_byte = root_block / 8;
  uint32_t root_bit = root_block % 8;
  buffer[root_byte] |= (1 << root_bit);

  if (!fs_write_block(superblock.bitmap_start, buffer))
    return false;

  //clear inode table blocks
  for(i = 0; i < FS_BLOCK_SIZE; i++)
    buffer[i] = 0;
  for(
    uint32_t block = 0;
    block < superblock.inode_blocks;
    block++) {
    if(!fs_write_block(superblock.inode_start + block, buffer))
      return false; 
  }

  //set up root inode
  uint8_t* root_bytes = (uint8_t*)&root_inode;
  for (i = 0; i < sizeof(fs_inode_t); i++) 
    root_bytes[i] = 0;

  root_inode.type = FS_TYPE_DIRECTORY;
  root_inode.size = 0;
  root_inode. links = 1;
  root_inode.blocks[0] = root_block;

  if (!fs_write_inode(0, &root_inode))
    return false;

  //empty root dir block
  for (i = 0; i < FS_BLOCK_SIZE; i++)
    buffer[i] = 0;
  if (!fs_write_block(root_block, buffer))
    return false; 

  return true;
}

int32_t fs_alloc_block() {
  uint8_t buffer[FS_BLOCK_SIZE];
  fs_superblock_t superblock;

  if (!fs_read_superblock(&superblock))
    return false;
  if (!fs_read_block(superblock.bitmap_start, buffer))
    return false;

  uint32_t i;
  for (i = 0; i < superblock.total_blocks; i++) {
    uint32_t is_reserved = (buffer[i / 8] & (1 << (i & 8)));
    if (!is_reserved) {
      buffer[i / 8] |= (1 << (i % 8));
      break;
    }
  }

  fs_write_block(superblock.bitmap_start, buffer);
  return i;
}

bool fs_free_block(uint32_t block) {
  uint8_t buffer[FS_BLOCK_SIZE];
  fs_superblock_t superblock;

  if (!fs_read_superblock(&superblock))
    return false;
  if (!fs_read_block(superblock.bitmap_start, buffer))
    return false;

  uint32_t i = block / FS_BLOCK_SIZE;
  buffer[i / 8] &= ~(1 << (i % 8));

  if (!fs_write_block(superblock.bitmap_start, buffer))
    return false;

  return true;
}

bool fs_read_inode(uint32_t inode_number, fs_inode_t* inode) {
  uint8_t buffer[FS_BLOCK_SIZE];
  fs_superblock_t superblock;
  if (!fs_read_superblock(&superblock))
    return false;

  if (inode_number >= superblock.inode_count)
    return false;

  uint32_t inodes_per_block = FS_BLOCK_SIZE / sizeof(fs_inode_t);
  uint32_t block_offset = inode_number / inodes_per_block;
  uint32_t inode_offset = inode_number % inodes_per_block;

  uint32_t block = superblock.inode_start + block_offset;

  if (!fs_read_block(block, buffer))
    return false;

  fs_inode_t* inodes = (fs_inode_t*)buffer;
  *inode = inodes[inode_offset];

  return true;
}

bool fs_write_inode(uint32_t inode_number, const fs_inode_t* inode) {
  fs_superblock_t superblock;
  uint8_t buffer[FS_BLOCK_SIZE];

  if (!fs_read_superblock(&superblock))
    return false;

  if (inode_number >= superblock.inode_count)
    return false;

  uint32_t inodes_per_block = FS_BLOCK_SIZE / sizeof(fs_inode_t);
  uint32_t block_offset = inode_number / inodes_per_block;
  uint32_t inode_offset = inode_number % inodes_per_block;

  uint32_t block = superblock.inode_start + block_offset;

  if (!fs_read_block(block, buffer))
    return false;

  fs_inode_t* inodes = (fs_inode_t*)buffer;
  inodes[inode_offset] = *inode;

  if (!fs_write_block(block, buffer))
    return false;

  return true;
}

int32_t fs_alloc_inode(uint8_t type) {
  fs_superblock_t superblock;
  fs_inode_t inode;

  if (!fs_read_superblock(&superblock))
    return -1;

  for (uint32_t i = 0; i < superblock.inode_count; i++) {
    if (!fs_read_inode(i, &inode))
      return -1;

    if (inode.type == FS_TYPE_FREE) {
      inode.type = type;
      inode.size = 0;
      inode.links = 1;

      for (int j = 0; j < 10; j++)
        inode.blocks[j] = 0;

      if (!fs_write_inode(i, &inode))
        return -1;

      superblock.free_inodes--;

      if(!fs_write_superblock(&superblock))
        return -1;

      return i;
    }
  }
  return -1;
}

bool fs_free_inode(uint32_t inode_number) {
  fs_superblock_t superblock;
  fs_inode_t inode;

  if (!fs_read_superblock(&superblock))
    return false;

  if (!fs_read_inode(inode_number, &inode))
    return false;

  for (int i = 0; i < FS_INODE_MAX_BLOCKS; i++) {
    if (inode.blocks[i] != 0) {
      if (!fs_free_block(inode.blocks[i]))
        return false;

      inode.blocks[i] = 0;
    }
  }

  inode.type = FS_TYPE_FREE;
  inode.size = 0;
  inode.links = 0;

  if (!fs_write_inode(inode_number, &inode))
    return false;

  superblock.free_inodes++;

  if (!fs_write_superblock(&superblock))
    return false;

  return true;
}

int32_t fs_find_directory_entry(uint32_t directory_inode, const char* name) {
  fs_inode_t directory;
  uint8_t buffer[FS_BLOCK_SIZE];

  if (!fs_read_inode(directory_inode, &directory))
    return -1;

  uint32_t entries_per_block = FS_BLOCK_SIZE / sizeof(fs_directory_entry_t);

  for (int i = 0; i < FS_INODE_MAX_BLOCKS; i++) {
    if (directory.blocks[i] == 0)
      break;

    if (!fs_read_block(directory.blocks[i], buffer))
      return -1;

    fs_directory_entry_t* entries = (fs_directory_entry_t*)buffer;

    for (uint32_t j = 0; j < entries_per_block; j++) {
      if (entries[j].inode == 0) 
        continue;

      int k = 0;

      while (
        name[k] != '\0' &&
        entries[j].name[k] != '\0' &&
        name[k] == entries[j].name[k]) {
        k++;
      }

      if (name[k] == '\0' && entries[j].name[k] == '\0')
        return entries[j].inode;
    }
  }
  return -1;
}

bool fs_add_directory_entry(uint32_t directory_inode, uint32_t inode_number, const char* name) {
  fs_inode_t directory;
  uint8_t buffer[FS_BLOCK_SIZE];

  if (!fs_read_inode(directory_inode, &directory))
    return false;

  uint32_t entries_per_block = FS_BLOCK_SIZE / sizeof(fs_directory_entry_t);
   
  for (int i = 0; i < FS_INODE_MAX_BLOCKS; i++) {
    //block is free so allocate the block
    if (directory.blocks[i] == 0) {
      int32_t block = fs_alloc_block();

      if (block < 0)
        return false;

      directory.blocks[i] = block;

      for (int j = 0; j < FS_BLOCK_SIZE; j++)
        buffer[j] = 0;

      if (!fs_write_block(block, (void*)buffer))
        return false;
    }
    
    if (!fs_read_block(directory.blocks[i], buffer))
      return false;

    fs_directory_entry_t* entries = (fs_directory_entry_t*)buffer;

    //search for free entry indicated by the .inode, if not found, will go to next iteration of 
    //block loop, if free, allocate it accordingly and then return true
    for (uint32_t j = 0; j < entries_per_block; j++) {
      if (entries[j].inode == 0) {
        entries[j].inode = inode_number;

        for (int k = 0; k < FS_FILENAME_LENGTH; k++)
          entries[j].name[k] = 0;

        for (int k = 0; k < FS_FILENAME_LENGTH - 1 && name[k] != '\0'; k++) {
          entries[j].name[k] = name[k]; 
        }

        if (!fs_write_block(directory.blocks[i], buffer))
          return false;

        directory.size += sizeof(fs_directory_entry_t);

        if (!fs_write_inode(directory_inode, &directory))
          return false;

        return true;
      }
    }
  }
  return false;
}

bool fs_remove_directory_entry(uint32_t directory_inode, const char* name) {
  fs_inode_t directory;
  uint8_t buffer[FS_BLOCK_SIZE];

  if(!fs_read_inode(directory_inode, &directory))
    return false;

  uint32_t entries_per_block = FS_BLOCK_SIZE / sizeof(fs_directory_entry_t);

  for (int i = 0; i < FS_INODE_MAX_BLOCKS; i++) {
    if (directory.blocks[i] == 0)
      break;

    if (!fs_read_block(directory.blocks[i], buffer))
      return false;

    fs_directory_entry_t* entries = (fs_directory_entry_t*)buffer;

    for (uint32_t j = 0; j < entries_per_block; j++) {
      if (entries[i].inode == 0)
        continue;

      int k = 0;

      while (name[k] != '\0' && 
        entries[j].name[k] != '\0' &&
        name[k] == entries[j].name[k]) {
        k++;
      }

      if (name[k] == '\0' && entries[j].name[k] == '\0') {
        //entry to remove found
        entries[j].inode = 0;
        
        if (!fs_write_block(directory.blocks[i], buffer))
          return false;

        return true;
      }
    }
  }
  return false;
}

int32_t fs_create_file(uint32_t directory_inode, const char* name) {
  if (fs_find_directory_entry(directory_inode, name) >= 0)
    return -1;

  int32_t file_inode = fs_alloc_inode(FS_TYPE_FILE);

  if (file_inode < 0)
    return -1;

  if (!fs_add_directory_entry(directory_inode, file_inode, name)) {
    fs_free_inode(file_inode);
    return -1;
  }

  return file_inode;
}

int32_t fs_create_directory(uint32_t parent_inode, const char* name) {
  if (fs_find_directory_entry(parent_inode, name) >= 0)
    return -1;

  int32_t dir_inode_number = fs_alloc_inode(FS_TYPE_DIRECTORY);

  if (dir_inode_number < 0)
    return -1;

  int32_t block = fs_alloc_block();

  if (block < 0) {
    fs_free_inode(dir_inode_number);
    return -1;
  }

  fs_inode_t dir_inode;

  if (!fs_read_inode(dir_inode_number, &dir_inode))
    return -1;

  for(int i = 0; i < FS_INODE_MAX_BLOCKS; i++)
    dir_inode.blocks[i] = 0;

  dir_inode.blocks[0] = block;
  dir_inode.size = 0;

  uint8_t buffer[FS_BLOCK_SIZE];

  //clear only block
  for (int i = 0; i < FS_BLOCK_SIZE; i++) 
    buffer[i] = 0;

  if (!fs_write_block(block, buffer))
    return -1;

  if (!fs_write_inode(dir_inode_number, &dir_inode))
    return -1;

  if (!fs_add_directory_entry(parent_inode, dir_inode_number, name))
    return -1;

  return dir_inode_number;
}

int32_t read_file(uint32_t file_inode_number, void* read_buffer, uint32_t size, uint32_t offset) {
  fs_inode_t file_inode;
  uint8_t buffer[FS_BLOCK_SIZE];

  if (!fs_read_inode(file_inode_number, &file_inode))
    return -1;

  if (offset >= file_inode.size)
    return -1;

  if (offset + size > file_inode.size)
    size = file_inode.size - offset;

  uint32_t bytes_read = 0;

  uint8_t* destination = (uint8_t*)read_buffer;

  while (bytes_read < size) {
    uint32_t position = offset + bytes_read;

    uint32_t block_index = position / FS_BLOCK_SIZE;
    uint32_t block_offset = position % FS_BLOCK_SIZE;

    if (block_index >= 10)
      break;

    if (!fs_read_block(file_inode.blocks[block_index], buffer))
        return -1;

    uint32_t bytes = FS_BLOCK_SIZE - block_offset;

    if (bytes > size - bytes_read)
      bytes = size - bytes_read;

    for (uint32_t i = 0; i < bytes; i++)
      destination[bytes_read + i] =
        buffer[block_offset + i];

    bytes_read += bytes;
  }

  return bytes_read;
}






bool init_fs() {
  init_ata();
  return true;
}
