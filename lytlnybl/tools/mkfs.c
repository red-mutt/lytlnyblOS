#include "filesystem/fs_layout.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <dirent.h>
#include <sys/stat.h>

static bool write_block(FILE *image, uint32_t block_num, const void *buffer) {
  uint32_t sector = FS_START_BLOCK + block_num;

  if (fseek(image, sector * FS_BLOCK_SIZE, SEEK_SET) != 0) 
    return false;
  return fwrite(buffer, 1, FS_BLOCK_SIZE, image) == FS_BLOCK_SIZE;
}

static bool read_block(FILE *image, uint32_t block_num, void *buffer) {
  uint32_t sector = FS_START_BLOCK + block_num;

  if (fseek(image, sector * FS_BLOCK_SIZE, SEEK_SET) != 0)
    return false;
  return fread(buffer, 1, FS_BLOCK_SIZE, image) == FS_BLOCK_SIZE;
}

static fs_superblock_t make_superblock(void) {
  fs_superblock_t superblock = {0};

  superblock.magic = FS_MAGIC;
  superblock.block_size = FS_BLOCK_SIZE;

  superblock.total_blocks = FS_TOTAL_BLOCKS;
  superblock.bitmap_start = FS_BITMAP_BLOCK;
  
  superblock.inode_start = FS_INODE_START;
  superblock.inode_count = FS_TOTAL_INODES;
  superblock.inode_blocks = (superblock.inode_count * sizeof(fs_inode_t) + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
  superblock.root_inode = FS_ROOT_INODE;

  superblock.data_start = superblock.inode_start + superblock.inode_blocks;

  // -1 for root dir remember
  superblock.free_blocks = superblock.total_blocks - superblock.data_start - 1;
  superblock.free_inodes = superblock.inode_count - 1;

  return superblock;
}

static bool write_superblock(FILE *image, const fs_superblock_t *superblock) {
  uint8_t buffer[FS_BLOCK_SIZE] = {0};
  memcpy(buffer, superblock, sizeof(fs_superblock_t));
  return write_block(image, FS_SUPERBLOCK, buffer);
}

static bool write_bitmap(FILE *image, const fs_superblock_t *superblock) {
  uint8_t bitmap[FS_BLOCK_SIZE] = {0};

  for (uint32_t block = 0; block < superblock->data_start; block++) {
    bitmap[block / 8] |= (1 << (block % 8));
  }

  uint32_t root_block = superblock->data_start;

  bitmap[root_block / 8] |= (1 << (root_block % 8));

  return write_block(image, superblock->bitmap_start, bitmap);
}

static bool clear_inode_table(FILE *image, const fs_superblock_t* superblock) {
  uint8_t buffer[FS_BLOCK_SIZE] = {0};

  for (size_t i = 0; i < superblock->inode_blocks; i++) {
    if (!write_block(image,superblock->inode_start + i, buffer)) {
      return false;
    }
  }

  return true;
}

static bool write_root_inode(FILE* image, const fs_superblock_t* superblock) {
  fs_inode_t root = {0};

  root.type = FS_TYPE_DIRECTORY;
  root.size = 0;
  root.blocks[0] = superblock->data_start;

  //pointless calcs for now as root is the first inode, but if that changes this will be useful
  uint32_t inodes_per_block = FS_BLOCK_SIZE / sizeof(fs_inode_t);
  uint32_t block_offset = FS_ROOT_INODE / inodes_per_block;
  uint32_t inode_offset = FS_ROOT_INODE % inodes_per_block;
  uint32_t block = superblock->inode_start + block_offset;

  uint8_t buffer[FS_BLOCK_SIZE];
  if (fseek(image, (block) * FS_BLOCK_SIZE, SEEK_SET) != 0){
    return false;
  }

  if (fread(buffer, 1, FS_BLOCK_SIZE, image) != FS_BLOCK_SIZE) {
    return false;
  }

  fs_inode_t *inodes = (fs_inode_t *)buffer;
  inodes[inode_offset] = root;

  return write_block(image, block, buffer);
}

static bool clear_root_directory(FILE *image, const fs_superblock_t* superblock) {
  uint8_t buffer[FS_BLOCK_SIZE] = {0};
  return write_block(image, superblock->data_start, buffer);
}

static bool format_filesystem(FILE *image) {
  fs_superblock_t superblock = make_superblock();

  if (!write_superblock(image, &superblock)) {
    fprintf(stderr, "mkfs: failed to write superblock\n");
    return false;
  }

  if (!write_bitmap(image, &superblock)) {
    fprintf(stderr, "mkfs: failed to write bitmap\n");
    return false;
  }

  if (!clear_inode_table(image, &superblock)) {
    fprintf(stderr, "mkfs: failed to clear inode table\n");
    return false;
  }

  if (!write_root_inode(image, &superblock)) {
    fprintf(stderr, "mkfs: failed to write root inode\n");
    return false;
  }

  return true;
}

/* -------------------------------------------
 *          REMAKE OF FS (needed) FUNCTIONS
 * -------------------------------------------
 */

int32_t alloc_block(FILE *image, fs_superblock_t* superblock) {
  uint8_t buffer[FS_BLOCK_SIZE];

  //read superblock
  if (!read_block(image, FS_SUPERBLOCK, buffer))
    return -1;
  memcpy(superblock, buffer, sizeof(fs_superblock_t));

  memset(buffer, 0, FS_BLOCK_SIZE);

  //read buffer
  if (!read_block(image, superblock->bitmap_start, buffer))
    return -1;

  size_t i;
  for (i = 0; i < superblock->total_blocks && i < (FS_BLOCK_SIZE * 8); i++) {
    uint32_t is_reserved = (buffer[i / 8] & (1 << (i % 8)));
    if (!is_reserved) {
      buffer[i / 8] |= (1 << (i % 8));
      break;
    }
  }

  if (i == superblock->total_blocks)
    return -1;

  if (!write_block(image, superblock->bitmap_start, buffer))
    return -1;
  superblock->free_blocks--;
  if (!write_superblock(image, superblock))
    return -1;
  return i;
}

bool write_inode(FILE* image, fs_superblock_t* superblock, uint32_t inode_num, const fs_inode_t* inode) {
  uint8_t buffer[FS_BLOCK_SIZE];

  if (!read_block(image, FS_SUPERBLOCK, buffer))
    return false;

  memcpy(superblock, buffer, sizeof(fs_superblock_t));

  if (inode_num >= superblock->inode_count)
    return false;

  uint32_t inodes_per_block = FS_BLOCK_SIZE / sizeof(fs_inode_t);
  uint32_t block_offset = inode_num / inodes_per_block;
  uint32_t inode_offset = inode_num % inodes_per_block;

  uint32_t block = superblock->inode_start + block_offset;

  if (!read_block(image, block, buffer))
    return false;

  fs_inode_t* inodes = (fs_inode_t*)buffer;
  inodes[inode_offset] = *inode;

  if (!write_block(image, block, buffer))
    return false;

  return true;
}

bool read_inode(FILE* image, fs_superblock_t* superblock, uint32_t inode_num, fs_inode_t* inode) {
  uint8_t buffer[FS_BLOCK_SIZE];

  if (!read_block(image, FS_SUPERBLOCK, buffer))
    return false;

  memcpy(superblock, buffer, sizeof(fs_superblock_t));
  memset(buffer, 0, FS_BLOCK_SIZE);

  if (inode_num >= superblock->inode_count)
    return false;

  uint32_t inodes_per_block = FS_BLOCK_SIZE / sizeof(fs_inode_t);
  uint32_t block_offset = inode_num / inodes_per_block;
  uint32_t inode_offset = inode_num % inodes_per_block;

  uint32_t block = superblock->inode_start + block_offset;

  if (!read_block(image, block, buffer))
    return false;

  fs_inode_t* inodes = (fs_inode_t*)buffer;
  *inode = inodes[inode_offset];

  return true;
}


bool add_directory_entry(FILE* image, fs_superblock_t* superblock, uint32_t directory_inode_num, uint32_t inode_num, 
    const char* name) {
  fs_inode_t directory;
  uint8_t buffer[FS_BLOCK_SIZE];

  if (!read_inode(image, superblock, directory_inode_num, &directory))
    return false;

  uint32_t entries_per_block = FS_BLOCK_SIZE / sizeof(fs_directory_entry_t);
   
  for (size_t i = 0; i < FS_INODE_MAX_BLOCKS; i++) {
    //no block is assigned so allocate a block
    if (directory.blocks[i] == 0) {
      int32_t block_num = alloc_block(image, superblock);

      if (block_num < 0)
        return false;

      directory.blocks[i] = block_num;

      memset(buffer, 0, FS_BLOCK_SIZE);

      if (!write_block(image, block_num, buffer)) {
        return false;
      }
    }
    
    if (!read_block(image, directory.blocks[i], buffer))
      return false;

    fs_directory_entry_t* entries = (fs_directory_entry_t*)buffer;

    //search for free entry indicated by the .inode, if not found, will go to next iteration of 
    //block loop, if free, allocate it accordingly and then return true
    for (size_t j = 0; j < entries_per_block; j++) {
      if (entries[j].inode == 0) {
        entries[j].inode = inode_num;

        memset(entries[j].name, 0, FS_FILENAME_LENGTH);

        strncpy(entries[j].name, name, FS_FILENAME_LENGTH - 1);

        if (!write_block(image, directory.blocks[i], buffer))
          return false;

        directory.size += sizeof(fs_directory_entry_t);

        if (!write_inode(image, superblock, directory_inode_num, &directory))
          return false;

        return true;
      }
    }
  }
  return false;
}



/* -----------------------------------------
 *          DIRECTORY PARSING 
 * -----------------------------------------
 */



uint32_t next_free_inode_num = 1;

static bool install_file(FILE* image, fs_superblock_t *sb, uint32_t parent_inode_num, const char* name,
    const char* host_path) {

  FILE *file = fopen(host_path, "rb");

  if (next_free_inode_num >= sb->inode_count) {
    fprintf(stderr, "mkfs: no free inode for %s\n", host_path);
    return false;
  }

  uint32_t inode_num = next_free_inode_num++;

  if (!file) {
    perror(host_path);
    return false;
  }

  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    return false;
  }

  size_t file_size = ftell(file);

  if (file_size < 0) {
    fclose(file);
    return false;
  }

  rewind(file);

  fs_inode_t file_inode = {0};

  file_inode.type = FS_TYPE_FILE;
  file_inode.size = (uint32_t)file_size;
  uint8_t buffer [FS_BLOCK_SIZE];

  uint32_t remaining = file_inode.size;

  //write data to blocks
  for (size_t i = 0; remaining > 0 && i < FS_INODE_MAX_BLOCKS; i++) {
    int32_t block_num = alloc_block(image, sb);

    if (block_num < 0) {
      fclose(file);
      return false;
    }

    file_inode.blocks[i] = block_num;

    memset(buffer, 0, FS_BLOCK_SIZE);

    uint32_t bytes = remaining > FS_BLOCK_SIZE ? FS_BLOCK_SIZE : remaining;

    if (fread(buffer, 1, bytes, file) != bytes) {
      fclose(file);
      return false;
    }

    if (!write_block(image, block_num, buffer)) {
      fclose(file);
      return false;
    }

    remaining -= bytes; 
  }

  fclose(file);

  if (remaining != 0)
    return false;

  if (!write_inode(image, sb, inode_num, &file_inode))
    return false;

  if (!add_directory_entry(image, sb, parent_inode_num, inode_num, name))
    return false;

  return true;


  
}


static bool install_directory(FILE* image, fs_superblock_t* sb, uint32_t parent_inode_num, const char* name,
    const char* host_path) {

  if (next_free_inode_num >= sb->inode_count) {
    fprintf(stderr, "mkfs: no free inode for the directory %s\n", host_path);
    return false;
  }

  uint32_t inode_num = next_free_inode_num++;
  fs_inode_t inode = {0};

  if (inode_num >= sb->inode_count) {
    fprintf(stderr, "mkfs: no free inode for the directory %s\n", host_path);
    return false;
  }

  int32_t block = alloc_block(image, sb);

  if (block < 0) {
    fprintf(stderr, "mkfs: no free block for directory %s\n", host_path);
    return false;
  }

  inode.type = FS_TYPE_DIRECTORY;
  inode.size = 0;
  inode.blocks[0] = block;

  if (!write_inode(image, sb, inode_num, &inode))
    return false;

  //write to it's parent and stuff
  fs_inode_t parent;

  if (!read_inode(image, sb, parent_inode_num, &parent))
    return false;

  if (!add_directory_entry(image, sb, parent_inode_num, inode_num, name))
    return false;
  

  //now install everything inside of directory same logic as rootfs install

  DIR *dir = opendir(host_path);
  if (!dir) {
    perror(host_path);
    return false;
  }

  struct dirent* entry;

  while ((entry = readdir(dir)) != NULL) { 
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    char path [512];
    
    snprintf(path, sizeof(path), "%s/%s", host_path, entry->d_name);

    struct stat st;

    if (stat(path, &st) != 0) {
      perror(path);
      closedir(dir);
      return false;
    }

    if (S_ISREG(st.st_mode)) {
      if (!install_file(image, sb, inode_num, entry->d_name, path)) {
        closedir(dir);
        return false;
      }

    } else if (S_ISDIR(st.st_mode)) {
      if (!install_directory(image, sb, inode_num, entry->d_name, path)) {
        closedir(dir);
        return false;
      }
    }
  }
  closedir(dir);
  return true;
}


static bool install_rootfs(FILE* image, fs_superblock_t *sb) {
  DIR *dir = opendir("rootfs");

  if (!dir) {
    perror("rootfs");
    return false;
  }
  
  struct dirent *entry;

  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    char path [512];
    
    snprintf(path, sizeof(path), "rootfs/%s", entry->d_name);

    struct stat st;

    if (stat(path, &st) != 0) {
      perror(path);
      closedir(dir);
      return false;
    }

    if (S_ISREG(st.st_mode)) {
      if (!install_file(image, sb, 0, entry->d_name, path)) {
        closedir(dir);
        return false;
      }
      
    } else if (S_ISDIR(st.st_mode)) {
      if (!install_directory(image, sb, 0, entry->d_name, path)) {
        closedir(dir);
        return false;
      }
    }
  }
  closedir(dir);
  return true;

}



int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "usage: mkfs <image>\n");
    return 1;
  }

  FILE *image = fopen(argv[1], "r+b");

  if (!image) {
    perror("mkfs: fopen");
    return 1;
  }

  printf("formatting %s..........\n", argv[1]);

  if (!format_filesystem(image)) {
    fprintf(stderr, "mkfs: formatting failed\n");
    fclose(image);
    return 1;
  }

  fs_superblock_t sb_copy;

  uint8_t buffer[FS_BLOCK_SIZE];

  if (!read_block(image, FS_SUPERBLOCK, buffer)) {
    fprintf(stderr, "mkfs: failed to read superblock\n");
    fclose(image);
    return 1;
  }

  memcpy(&sb_copy, buffer, sizeof(fs_superblock_t));

  if (!install_rootfs(image, &sb_copy)){
    fprintf(stderr, "mkfs: failed to install rootfs\n");
    fclose(image);
    return 1;
  }

  fclose(image);

  printf("mkfs success\n");

  return 0;
}
