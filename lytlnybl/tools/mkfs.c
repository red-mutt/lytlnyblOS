#include "filesystem/fs_layout.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

static bool write_block(FILE *image, uint32_t block_num, const void *buffer) {
  uint32_t sector = FS_START_BLOCK + block_num;

  if (fseek(image, sector * FS_BLOCK_SIZE, SEEK_SET) != 0) {
    return false;
  }
  if (fwrite(buffer, 1, FS_BLOCK_SIZE, image) != FS_BLOCK_SIZE) {
    return false;
  }
  return true;
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
  if (fseek(image, (block + FS_START_BLOCK) * FS_BLOCK_SIZE, SEEK_SET) != 0){
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

  fclose(image);

  printf("filesystem format success\n");

  return 0;
}
