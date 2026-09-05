# The Filesystem Core

This will be a long and grueling task. My filesystems code is around 700 lines,
so we will be writing a lot of code in one go. I already covered how the filesystem
should work in our Introduction, so let's get straight in to writing code and 
tackling this long task.

## Kernel Utilities

Before we get started on writing, I want to make a file for kernel utilities.
One of these utilities already exists from when we wrote our interrupts, it's `memset()`.
Alongside moving `memset` to this file, we will also create `memcpy`, `memcmp` and `strncpy`,
these will all get used frequently within the code for the filesystem.

Here's the header:

```c
#ifndef KERNEL_UTILS_H
#define KERNEL_UTILS_H

#include <stddef.h>
#include <stdint.h>

void* memset(void* dest, uint8_t val, size_t len);
void* memcpy(void* dest, const void* src, size_t len);
int32_t strcmp(const char* a, const char* b);
char* strncpy(char* dest, const char* src, size_t n);

#endif
```

And the implementation:

```c
#include "kernel/kernel_utils.h"
 
void* memset(void* dest, uint8_t val, size_t len) {
  uint8_t* d = (uint8_t*)dest;
 
  for (size_t i = 0; i < len; i++)
    d[i] = val;
 
  return dest;
}
 
void* memcpy(void* dest, const void* src, size_t len) {
  uint8_t* d = (uint8_t*)dest;
  const uint8_t* s = (const uint8_t*)src;
 
  for (size_t i = 0; i < len; i++)
    d[i] = s[i];
 
  return dest;
}
 
int32_t strcmp(const char* a, const char* b) {
  size_t i = 0;
 
  while (a[i] != '\0' && b[i] != '\0' && a[i] == b[i])
    i++;
 
  return (uint8_t)a[i] - (uint8_t)b[i];
}
 
char* strncpy(char* dest, const char* src, size_t n) {
  size_t i = 0;
 
  for (; i < n && src[i] != '\0'; i++)
    dest[i] = src[i];
 
  for (; i < n; i++)
    dest[i] = '\0';
 
  return dest;
}```

I won't explain these, we wrote things pretty similar to these functions when we wrote the
minimal libc. 

## Two header files

Our implementation for the filesystem will have two header files. One called 
`fs_layout.h` that contains the definitions and structures for the filesystem, 
and one called `fs.h` that contains the functions for the filesystem.
Why do we do this? This is because later, in the programs section, we will make 
a program called MKFS (make filesystem) which will populate our filesystem with
directories and files before the operating system runs and will require `fs_layout.h`.

Let's look at `fs_layout.h`:

```c
#ifndef FS_LAYOUT_H
#define FS_LAYOUT_H

#include <stdint.h>

#define FS_BLOCK_SIZE 512

#define FS_START_BLOCK 70
#define FS_SUPERBLOCK 0 // 1 block
#define FS_BITMAP_BLOCK 1 //1 block
#define FS_INODE_START 2 //block count calculated when formatting

#define FS_FILENAME_LENGTH 32

#define FS_TYPE_FREE 0
#define FS_TYPE_FILE 1
#define FS_TYPE_DIRECTORY 2

#define FS_INODE_MAX_BLOCKS 10
#define FS_TOTAL_BLOCKS 1000
#define FS_TOTAL_INODES 128

#define FS_ROOT_INODE 0

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
```

### The Definitions

Let's cover the definitions first. `BLOCK_SIZE` is self-explanatory, the next couple
define locations in the filesystem. `START_BLOCK` states that the whole filesystem
should start at the seventieth block in the disk image, this gives ample space for the
filesystem without overwriting the kernel. The next 3 locations are all then relative to this
start block.

After our locations, there is the definition for the max file name that a directory can
contain. Next, there is our
inode types: `FREE` an inode to be allocated, `DIRECTORY` for directories and `FILE` for
files.

Next we have some more definitions of the maximums for our filesystem. The maximum
number of blocks that an inode can point to, the maximum number of blocks that an inode can point to
and finally the total number of inodes for the whole filesystem.

>**_NOTE:_** When writing any programs, you should be mindful of the 
`FS_INODE_MAX_BLOCKS` value. `10 x 512 bytes` is 5120 bytes, so this will become
the greatest size for any programs or files you eventually store in the filesystem 
unless you change this.

`FS_ROOT_INODE` is the inode index for the root directory that would be made during
initialization as you cannot store anything without a root directory.
`FS_MAGIC` is our filesystem's signature. This is a value we check to ensure
that the filesystem we write to and read from is actually the one we have programmed.

### The structures

Our first structure is the superblock, this will be read from a lot in 
our implementation's code. Most of its data mirrors the previous definitions.
The superblock contains: 

-   The magic number (signature) for our filesystem
-   size of each block and the total blocks
-   The start of the bitmap, mirrors `FS_BITMAP_BLOCK`
-   The start of the inode block, the count of inodes (128), 
    the number of blocks that an inode can link to (10) and 
    the index for the root inode.
-   And then we have data that will get frequently updated such as
    the number of free blocks and free inodes

The next structure is for inodes. It contains the size in bytes for the total count
of data that is written to an inode's blocks, the inode type, and a list
of indexes that links to each block.

The final structure defines entries for directories; it has a number for 
inode it relates to and a name. You may potentially be confused on what a directory actually is:
It's just an inode, like a file, but instead of its blocks linking to data, blocks are just
an array of directory entries.

### Functions

Here's the other header:

```c
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
```

Now that's a lot of functions, try not to get overwhelmed. Most of these functions 
build from earlier ones as we go down. For example: `fs_alloc_inode` will use `fs_write_inode` and this
will use both `fs_read_block` and `fs_write_block`.

Let's look quickly at the abstracted functionality of each.

-   `fs_read_block` and `fs_write_block` directly calls the reading and writing 
    functions that we wrote in our driver. Also adds the offset of `FS_START_BLOCK`
    when reading and writing.
-   `fs_write_superblock` and `fs_read_superblock` does the same as the previous but specifically
    reads and writes the superblock structure given to it.
-   `fs_format` sets up a new filesystem on the disk.

Block allocation. These two will be adjacent to the code in the PMM.

-   `fs_alloc_block` finds a free block on the `kernel.img` and marks it as used, then 
    returns the number of the block.
-   `fs_free_block` marks a previously allocated block as free.

The next group handles inodes:
-   `fs_read_inode` reads an inode from the inode table.
-   `fs_write_inode` whites an inode to the inode table.
-   `fs_alloc_inode` finds a free inode and sets it up.
-   `fs_free_inode` marks an inode as free.

We then have the functions for directories:

- `fs_find_directory_entry` looks for a filename inside a directory and
  returns the inode associated with it.
- `fs_add_directory_entry` adds a filename and inode number to a directory.
- `fs_remove_directory_entry` removes a filename from a directory.

Finally, we have the higher-level file operations:

- `fs_create_file` creates a new file and adds it to a directory.
- `fs_create_directory` creates a new directory and adds it to its parent.
- `fs_read_file` reads data from a file.
- `fs_write_file` writes data to a file.
- `fs_delete_file` removes a file from a directory and frees the resources
  associated with it.


The important thing to notice is that all the functions become more
high-level as we go down the list. For example, `fs_read_block` deals directly with disk blocks,
while `fs_read_file` can read part of a file without you needing to know where on `kernel.img` the file's
data is stored.

## Writing the functions

I will be explaining around 600 lines of code all in one go. Brace
yourself and try not to get too overwhelmed.

```c
#include "filesystem/fs.h"
#include "kernel/drivers/ata.h"
#include "kernel/kernel_utils.h"

bool fs_read_block(uint32_t block_num, void* buffer) {
  uint32_t sector = FS_START_BLOCK + block_num;
  return ata_read_sector(sector, buffer);
}

bool fs_write_block(uint32_t block_num, const void* buffer) {
  uint32_t sector = FS_START_BLOCK + block_num;
  return ata_write_sector(sector, buffer);
}

bool fs_write_superblock(const fs_superblock_t* superblock) {
  uint8_t buffer[FS_BLOCK_SIZE];

  memset(buffer, 0, FS_BLOCK_SIZE);

  memcpy(buffer,superblock, sizeof(fs_superblock_t));

  return fs_write_block(FS_SUPERBLOCK, buffer);
}

bool fs_read_superblock(fs_superblock_t* superblock) {
  uint8_t buffer[FS_BLOCK_SIZE];

  if (!fs_read_block(FS_SUPERBLOCK, buffer))
    return false;

  memcpy(superblock, buffer, sizeof(fs_superblock_t));

  if (superblock->magic != FS_MAGIC)
    return false;

  return true;
}

bool fs_format(void) {
  fs_superblock_t superblock;
  fs_inode_t root_inode;
  uint8_t buffer[FS_BLOCK_SIZE];

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


  
  if (!fs_write_superblock(&superblock))
    return false;

  memset(buffer, 0, FS_BLOCK_SIZE);

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
  memset(buffer, 0, FS_BLOCK_SIZE);
  for(
    uint32_t block = 0;
    block < superblock.inode_blocks;
    block++) {
    if(!fs_write_block(superblock.inode_start + block, buffer))
      return false; 
  }

  //set up root inode
  memset(&root_inode, 0, sizeof(fs_inode_t));

  root_inode.type = FS_TYPE_DIRECTORY;
  root_inode.size = 0;
  root_inode.blocks[0] = root_block;

  if (!fs_write_inode(superblock.root_inode, &root_inode))
    return false;

  //empty root dir block
  memset(buffer, 0, FS_BLOCK_SIZE);
  if (!fs_write_block(root_block, buffer))
    return false; 

  return true;
}

int32_t fs_alloc_block(void) {
  uint8_t buffer[FS_BLOCK_SIZE];
  fs_superblock_t superblock;

  if (!fs_read_superblock(&superblock))
    return -1;
  if (!fs_read_block(superblock.bitmap_start, buffer))
    return -1;

  size_t i;
  for (i = 0; i < superblock.total_blocks && i < (FS_BLOCK_SIZE * 8); i++) {
    uint32_t is_reserved = (buffer[i / 8] & (1 << (i % 8)));
    if (!is_reserved) {
      buffer[i / 8] |= (1 << (i % 8));
      break;
    }
  }

  if (i == superblock.total_blocks)
    return -1;

  if (!fs_write_block(superblock.bitmap_start, buffer))
    return -1;
  superblock.free_blocks--;
  if (!fs_write_superblock(&superblock))
    return -1;
  return i;
}

bool fs_free_block(uint32_t block) {
  uint8_t buffer[FS_BLOCK_SIZE];
  fs_superblock_t superblock;

  if (!fs_read_superblock(&superblock))
    return false;

  if (block >= superblock.total_blocks)
    return false;

  if (!fs_read_block(superblock.bitmap_start, buffer))
    return false;


  uint32_t byte = block / 8;
  uint32_t bit = block % 8;

  buffer[byte] &= ~(1 << bit);

  superblock.free_blocks++;

  if (!fs_write_block(superblock.bitmap_start, buffer))
    return false;

  if (!fs_write_superblock(&superblock))
    return false;

  return true;
}

bool fs_read_inode(uint32_t inode_num, fs_inode_t* inode) {
  uint8_t buffer[FS_BLOCK_SIZE];
  fs_superblock_t superblock;
  if (!fs_read_superblock(&superblock))
    return false;

  if (inode_num >= superblock.inode_count)
    return false;

  uint32_t inodes_per_block = FS_BLOCK_SIZE / sizeof(fs_inode_t);
  uint32_t block_offset = inode_num / inodes_per_block;
  uint32_t inode_offset = inode_num % inodes_per_block;

  uint32_t block = superblock.inode_start + block_offset;

  if (!fs_read_block(block, buffer))
    return false;

  fs_inode_t* inodes = (fs_inode_t*)buffer;
  *inode = inodes[inode_offset];

  return true;
}

bool fs_write_inode(uint32_t inode_num, const fs_inode_t* inode) {
  fs_superblock_t superblock;
  uint8_t buffer[FS_BLOCK_SIZE];

  if (!fs_read_superblock(&superblock))
    return false;

  if (inode_num >= superblock.inode_count)
    return false;

  uint32_t inodes_per_block = FS_BLOCK_SIZE / sizeof(fs_inode_t);
  uint32_t block_offset = inode_num / inodes_per_block;
  uint32_t inode_offset = inode_num % inodes_per_block;

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

  for (size_t i = 0; i < superblock.inode_count; i++) {
    if (!fs_read_inode(i, &inode))
      return -1;

    if (inode.type == FS_TYPE_FREE) {
      inode.type = type;
      inode.size = 0;

      memset(inode.blocks, 0, sizeof(inode.blocks));

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

bool fs_free_inode(uint32_t inode_num) {
  fs_superblock_t superblock;
  fs_inode_t inode;
  bool failure = false;

  if (!fs_read_inode(inode_num, &inode))
    return false;

  for (size_t i = 0; i < FS_INODE_MAX_BLOCKS; i++) {
    if (inode.blocks[i] != 0) {
      //need a variable to represent failure, this is becuase we don't wanna return halfway though writing
      //on a failure because this will corrupt the filesystem.
      if (!fs_free_block(inode.blocks[i]))
        failure = true;

      inode.blocks[i] = 0;
    }
  }

  if (failure)
    return false;

  inode.type = FS_TYPE_FREE;
  inode.size = 0;

  if (!fs_write_inode(inode_num, &inode))
    return false;

  if (!fs_read_superblock(&superblock))
    return false;

  superblock.free_inodes++;

  if (!fs_write_superblock(&superblock))
    return false;

  return true;
}

int32_t fs_find_directory_entry(uint32_t directory_inode_num, const char* name) {
  fs_inode_t directory;
  uint8_t buffer[FS_BLOCK_SIZE];

  if (!fs_read_inode(directory_inode_num, &directory))
    return -1;

  uint32_t entries_per_block = FS_BLOCK_SIZE / sizeof(fs_directory_entry_t);

  for (size_t i = 0; i < FS_INODE_MAX_BLOCKS; i++) {
    if (directory.blocks[i] == 0)
      break;

    if (!fs_read_block(directory.blocks[i], buffer))
      return -1;

    fs_directory_entry_t* entries = (fs_directory_entry_t*)buffer;

    for (uint32_t j = 0; j < entries_per_block; j++) {
      if (entries[j].inode == 0) 
        continue;

      if (strcmp(name, entries[j].name) == 0)
        return entries[j].inode;
    }

  }
  return -1;
}

bool fs_add_directory_entry(uint32_t directory_inode_num, uint32_t inode_num, const char* name) {
  fs_inode_t directory;
  uint8_t buffer[FS_BLOCK_SIZE];

  if (!fs_read_inode(directory_inode_num, &directory))
    return false;

  uint32_t entries_per_block = FS_BLOCK_SIZE / sizeof(fs_directory_entry_t);
   
  for (size_t i = 0; i < FS_INODE_MAX_BLOCKS; i++) {
    //no block is assigned so allocate a block
    if (directory.blocks[i] == 0) {
      int32_t block_num = fs_alloc_block();

      if (block_num < 0)
        return false;

      directory.blocks[i] = block_num;

      memset(buffer, 0, FS_BLOCK_SIZE);

      if (!fs_write_block(block_num, buffer)) {
        fs_free_block(block_num);
        return false;
      }
    }
    
    if (!fs_read_block(directory.blocks[i], buffer))
      return false;

    fs_directory_entry_t* entries = (fs_directory_entry_t*)buffer;

    //search for free entry indicated by the .inode, if not found, will go to next iteration of 
    //block loop, if free, allocate it accordingly and then return true
    for (size_t j = 0; j < entries_per_block; j++) {
      if (entries[j].inode == 0) {
        entries[j].inode = inode_num;

        memset(entries[j].name, 0, FS_FILENAME_LENGTH);

        strncpy(entries[j].name, name, FS_FILENAME_LENGTH - 1);

        if (!fs_write_block(directory.blocks[i], buffer))
          return false;

        directory.size += sizeof(fs_directory_entry_t);

        if (!fs_write_inode(directory_inode_num, &directory))
          return false;

        return true;
      }
    }
  }
  return false;
}

bool fs_remove_directory_entry(uint32_t directory_inode_num, const char* name) {
  fs_inode_t directory;
  uint8_t buffer[FS_BLOCK_SIZE];

  if(!fs_read_inode(directory_inode_num, &directory))
    return false;

  uint32_t entries_per_block = FS_BLOCK_SIZE / sizeof(fs_directory_entry_t);

  for (size_t i = 0; i < FS_INODE_MAX_BLOCKS; i++) {
    if (directory.blocks[i] == 0)
      break;

    if (!fs_read_block(directory.blocks[i], buffer))
      return false;

    fs_directory_entry_t* entries = (fs_directory_entry_t*)buffer;

    for (uint32_t j = 0; j < entries_per_block; j++) {

      if (entries[j].inode == 0)
        continue;

      if (strcmp(name, entries[j].name) == 0) {
        //entry to remove found
        entries[j].inode = 0;
        
        if (!fs_write_block(directory.blocks[i], buffer))
          return false;

        directory.size -= sizeof(fs_directory_entry_t);

        if (!fs_write_inode(directory_inode_num, &directory))
          return false;

        return true;
      }
    }
  }
  return false;
}

int32_t fs_create_file(uint32_t directory_inode_num, const char* name) {
  if (fs_find_directory_entry(directory_inode_num, name) >= 0)
    return -1;

  int32_t file_inode = fs_alloc_inode(FS_TYPE_FILE);

  if (file_inode < 0)
    return -1;

  if (!fs_add_directory_entry(directory_inode_num, file_inode, name)) {
    fs_free_inode(file_inode);
    return -1;
  }

  return file_inode;
}

int32_t fs_create_directory(uint32_t parent_inode_num, const char* name) {
  if (fs_find_directory_entry(parent_inode_num, name) >= 0)
    return -1;

  int32_t dir_inode_num = fs_alloc_inode(FS_TYPE_DIRECTORY);

  if (dir_inode_num < 0)
    return -1;

  int32_t block_num = fs_alloc_block();

  if (block_num < 0) {
    fs_free_inode(dir_inode_num);
    return -1;
  }

  fs_inode_t dir_inode;

  if (!fs_read_inode(dir_inode_num, &dir_inode))
    goto fail;

  memset(dir_inode.blocks, 0, sizeof(dir_inode.blocks));

  dir_inode.blocks[0] = block_num;
  dir_inode.size = 0;

  uint8_t buffer[FS_BLOCK_SIZE];

  memset(buffer, 0, FS_BLOCK_SIZE);

  if (!fs_write_block(block_num, buffer))
    goto fail;

  if (!fs_write_inode(dir_inode_num, &dir_inode))
    goto fail;

  if (!fs_add_directory_entry(parent_inode_num, dir_inode_num, name))
    goto fail;

  return dir_inode_num;

fail:
  fs_free_block(block_num);
  fs_free_inode(dir_inode_num);
  return -1;
}

int32_t fs_read_file(uint32_t file_inode_num, void* read_buffer, uint32_t size, uint32_t offset) {
  fs_inode_t file_inode;
  uint8_t buffer[FS_BLOCK_SIZE];

  if (!fs_read_inode(file_inode_num, &file_inode))
    return -1;

  if (offset >= file_inode.size)
    return 0; //EOF should return 0 bytes instead of an error

  if (offset + size > file_inode.size)
    size = file_inode.size - offset;

  uint32_t bytes_read = 0;

  uint8_t* destination = (uint8_t*)read_buffer;

  while (bytes_read < size) {
    uint32_t position = offset + bytes_read;

    uint32_t block_index = position / FS_BLOCK_SIZE;
    uint32_t block_offset = position % FS_BLOCK_SIZE;

    if (block_index >= FS_INODE_MAX_BLOCKS)
      break;

    if (!fs_read_block(file_inode.blocks[block_index], buffer))
        return -1;

    uint32_t bytes = FS_BLOCK_SIZE - block_offset;

    if (bytes > size - bytes_read)
      bytes = size - bytes_read;

    memcpy(destination + bytes_read, buffer + block_offset, bytes);

    bytes_read += bytes;
  }

  return bytes_read;
}

int32_t fs_write_file(uint32_t inode_num, const void* write_buffer, uint32_t size, uint32_t offset) {
  fs_inode_t file_inode;
  uint8_t buffer[FS_BLOCK_SIZE];

  if (!fs_read_inode(inode_num, &file_inode))
    return -1;

  const uint8_t* source = (const uint8_t*)write_buffer;
  uint32_t bytes_written = 0;

  while (bytes_written < size) {
    uint32_t position = offset + bytes_written;

    uint32_t block_index = position / FS_BLOCK_SIZE;
    uint32_t block_offset = position % FS_BLOCK_SIZE;

    if (block_index >= FS_INODE_MAX_BLOCKS)
      break;

    //block not allocated, so allocate one and make it free, if else just read block.
    if (file_inode.blocks[block_index] == 0) {
      int32_t block = fs_alloc_block();

      if (block < 0)
        return -1;

      file_inode.blocks[block_index] = block;

      memset(buffer, 0, FS_BLOCK_SIZE);
    } else {
      if (!fs_read_block(file_inode.blocks[block_index], buffer))
          return -1;
    }

    uint32_t bytes = FS_BLOCK_SIZE - block_offset;

    if (bytes > size - bytes_written)
      bytes = size - bytes_written;

    memcpy(buffer + block_offset, source + bytes_written, bytes);

    if (!fs_write_block(file_inode.blocks[block_index], buffer))
      return -1;

    bytes_written += bytes;
  }

  if (offset + bytes_written > file_inode.size)
    file_inode.size = offset + bytes_written;

  if (!fs_write_inode(inode_num, &file_inode))
    return -1;

  return bytes_written;
}

bool fs_delete_file(uint32_t directory_inode, const char* name) {
  int32_t inode_num = fs_find_directory_entry(directory_inode, name);

  if (inode_num < 0)
    return false;

  if (!fs_remove_directory_entry(directory_inode, name))
    return false;

  if (!fs_free_inode(inode_num))
    return false;

  return true;
}
```

### `fs_read_block`

```c
bool fs_read_block(uint32_t block_num, void* buffer) {
  uint32_t sector = FS_START_BLOCK + block_num;
  return ata_read_sector(sector, buffer);
```

This calls `ata_read_sector` and returns the value. We also add the position
of `FS_START_BLOCK` to `block_num`, this is because throughout the filesystem
code we want to treat the zeroth block as the start of the filesystem.

### `fs_write_block`

```c
bool fs_write_block(uint32_t block_num, const void* buffer) {
  uint32_t sector = FS_START_BLOCK + block_num;
  return ata_write_sector(sector, buffer);
}
```

Does the exact same as the previous but does it for writing. The buffer 
gets passed as a constant as we wouldn't ever want to change data that is being written
to the filesystem.

### `fs_write_superblock`

```c
bool fs_write_superblock(const fs_superblock_t* superblock) {
  uint8_t buffer[FS_BLOCK_SIZE];

  memset(buffer, 0, FS_BLOCK_SIZE);

  memcpy(buffer,superblock, sizeof(fs_superblock_t));

  return fs_write_block(FS_SUPERBLOCK, buffer);
}
```

For this function we create a 512 byte temporary block and clear it. The superblock
structure is then copied to this temporary buffer, which is then written to the filesystem at the
superblock location.

### `fs_read_superblock`

```c
bool fs_read_superblock(fs_superblock_t* superblock) {
  uint8_t buffer[FS_BLOCK_SIZE];

  if (!fs_read_block(FS_SUPERBLOCK, buffer))
    return false;

  memcpy(superblock, buffer, sizeof(fs_superblock_t));

  if (superblock->magic != FS_MAGIC)
    return false;

  return true;
}
```

Here, we read the block at the superblock location into the buffer and copy
this data into the `fs_superblock_t` data structure passed to the function.
After this we also check the magic variable in the superblock to ensure
that the superblock has been read properly and that we are reading from the 
correct filesystem.

### `fs_format`

```c
bool fs_format(void) {
  fs_superblock_t superblock;
  fs_inode_t root_inode;
  uint8_t buffer[FS_BLOCK_SIZE];

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


  
  if (!fs_write_superblock(&superblock))
    return false;

  memset(buffer, 0, FS_BLOCK_SIZE);

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
  memset(buffer, 0, FS_BLOCK_SIZE);
  for(
    uint32_t block = 0;
    block < superblock.inode_blocks;
    block++) {
    if(!fs_write_block(superblock.inode_start + block, buffer))
      return false; 
  }

  //set up root inode
  memset(&root_inode, 0, sizeof(fs_inode_t));

  root_inode.type = FS_TYPE_DIRECTORY;
  root_inode.size = 0;
  root_inode.blocks[0] = root_block;

  if (!fs_write_inode(superblock.root_inode, &root_inode))
    return false;

  //empty root dir block
  memset(buffer, 0, FS_BLOCK_SIZE);
  if (!fs_write_block(root_block, buffer))
    return false; 

  return true;
}
```

This function uses `fs_write_inode` which have not yet explained, all you need to 
know is that this writes an inode to the filesystem at a given inode index.

Before I explain, let's look at the resulting layout.

```
Filesystem blocks:
    0   Superblock
    1   Bitmap
  2-13  Inode Table
   14   Root directory
 15-999 Available data blocks
```

#### Step 1: Building the superblock

The filesystem starts at sector 70, so on the actual disk image these are offsets
from 70. The result of this function is having a set-up filesystem with a single root directory.
All the variables get initialized to their definition counterparts. Other than `inode_blocks`,
this variable represents the count of blocks that are dedicated to the inode table and is 
calculated by doing `(128 inodes x size of one inode) / 512 = number of blocks`.
`FS_BLOCK_SIZE - 1` is done because we are doing ceiling division, this ensures that division always
rounds up because if we had 513 bytes, this would require 2 blocks and not 1.

`superblock.data_start` must be put right after the inode table so it is calculated by
`superblock.inode_start + superblock.inode_blocks`. `free_blocks` gets calculated
by having the blocks taken up by the superblock, bitmap, inode table, and root directory (to be created)
taken away from the `total_blocks` value. After all this data is stored in the superblock, we then write it to
the superblock location.

#### Step 2: Create the bitmap

Next is building the bitmap. 0 means a block is free, 1 means a block is reserved.
The loop marks blocks before `data_start` as used because they contain filesystem 
metadata.

#### Step 3: Clear the inode table

Our filesystem needs every inode to begin as a free inode. Because `FS_TYPE_FREE` is
0, clearing the inode table makes all 128 inodes free.

#### Step 4: Create root inode & its data

Inode 0 is our inode index for the root directory, its type is `FS_TYPE_DIRECTORY`, 
the first block it points to is block 14, and it establishes the filesystem's starting 
directory. Block 14 must also be cleared so that the root directory is interpreted
as empty.

### `fs_alloc_block`

```c
int32_t fs_alloc_block(void) {
  uint8_t buffer[FS_BLOCK_SIZE];
  fs_superblock_t superblock;

  if (!fs_read_superblock(&superblock))
    return -1;
  if (!fs_read_block(superblock.bitmap_start, buffer))
    return -1;

  size_t i;
  for (i = 0; i < superblock.total_blocks && i < (FS_BLOCK_SIZE * 8); i++) {
    uint32_t is_reserved = (buffer[i / 8] & (1 << (i % 8)));
    if (!is_reserved) {
      buffer[i / 8] |= (1 << (i % 8));
      break;
    }
  }

  if (i == superblock.total_blocks)
    return -1;

  if (!fs_write_block(superblock.bitmap_start, buffer))
    return -1;
  superblock.free_blocks--;
  if (!fs_write_superblock(&superblock))
    return -1;
  return i;
}
```

The steps for block allocation goes as follows:
1.  Read the superblock.
2.  Read the bitmap.
3.  Search for the first clear bit.
4.  Set that bit
5.  Write the bitmap block.
6.  Decrease the free-block count.
7.  Write the superblock back.
8.  Return the filesystem block number that we have just allocated

If you're confused about how `buffer[i / 8]` and `i << (1 % 8)` access a bit
in a byte. Imagine we want to access block 10, `10 / 8 = 1` and `10 % 8 = 2`, so 
block 10 gets represented by bit 2 of byte 1. The loop has the condition `i < FS_BLOCK_SIZE * 8`
because the bitmap only belongs to one block, so we don't want to iterate over the size
of the bitmap.

### `fs_free_block`

```c
bool fs_free_block(uint32_t block) {
  uint8_t buffer[FS_BLOCK_SIZE];
  fs_superblock_t superblock;

  if (!fs_read_superblock(&superblock))
    return false;

  if (block >= superblock.total_blocks)
    return false;

  if (!fs_read_block(superblock.bitmap_start, buffer))
    return false;


  uint32_t byte = block / 8;
  uint32_t bit = block % 8;

  buffer[byte] &= ~(1 << bit);

  superblock.free_blocks++;

  if (!fs_write_block(superblock.bitmap_start, buffer))
    return false;

  if (!fs_write_superblock(&superblock))
    return false;

  return true;
}
```

This is just the inverse of the previous allocation, we:

1.  Validate that the block exists.
2.  Locate its bit in the bitmap.
3.  Clear the bit
4.  Increase `free_blocks`.
5.  Write the updated bitmap and superblock.

The bitwise operation that sets the bit to 0 may be confusing for you. 
It essentially creates a byte where every bit is 1 other than the bit that we 
are setting to 0 and then performs a bitwise and against the byte we are changing.

### `fs_read_inode`

```c
bool fs_read_inode(uint32_t inode_num, fs_inode_t* inode) {
  uint8_t buffer[FS_BLOCK_SIZE];
  fs_superblock_t superblock;
  if (!fs_read_superblock(&superblock))
    return false;

  if (inode_num >= superblock.inode_count)
    return false;

  uint32_t inodes_per_block = FS_BLOCK_SIZE / sizeof(fs_inode_t);
  uint32_t block_offset = inode_num / inodes_per_block;
  uint32_t inode_offset = inode_num % inodes_per_block;

  uint32_t block = superblock.inode_start + block_offset;

  if (!fs_read_block(block, buffer))
    return false;

  fs_inode_t* inodes = (fs_inode_t*)buffer;
  *inode = inodes[inode_offset];

  return true;
}
```

This function basically provides a mapping from an inode number to its physical
location on the inode table, there are two pieces: `block_offset` and `inode_offset`
`block_offset` is the block that the inode is contained within and `inode_offset` is the
index for the inode inside this block. `block = superblock.inode_start + block_offset`
converts the inode-table-relative block into the filesystem block containing that inode.
The cast: `fs_inode_t* inodes = (fs_inode_t*)buffer;` then allows us to view
the 512-byte block as an array of `fs_inode_t` of which we when retrieve the specified
inode to be read from with the `inode_offset`.

### `fs_write_inode`

```c
bool fs_write_inode(uint32_t inode_num, const fs_inode_t* inode) {
  fs_superblock_t superblock;
  uint8_t buffer[FS_BLOCK_SIZE];

  if (!fs_read_superblock(&superblock))
    return false;

  if (inode_num >= superblock.inode_count)
    return false;

  uint32_t inodes_per_block = FS_BLOCK_SIZE / sizeof(fs_inode_t);
  uint32_t block_offset = inode_num / inodes_per_block;
  uint32_t inode_offset = inode_num % inodes_per_block;

  uint32_t block = superblock.inode_start + block_offset;

  if (!fs_read_block(block, buffer))
    return false;

  fs_inode_t* inodes = (fs_inode_t*)buffer;
  inodes[inode_offset] = *inode;

  if (!fs_write_block(block, buffer))
    return false;

  return true;
}
```

With the operating of writing an inode to the inode table, we cannot simply just
overwrite the entire block with a new inode, because one block contains multiple inodes.
Therefore, it:
1.  Reads the existing inode-table block.
2.  Changes only the selected inode in the buffer.
3.  Writes the entire block back.
This prevents the other inodes stored in the same block from being destroyed. A lot of 
the code in this function is similar to the previous one for reading.

### `fs_alloc_inode`

```c
int32_t fs_alloc_inode(uint8_t type) {
  fs_superblock_t superblock;
  fs_inode_t inode;

  if (!fs_read_superblock(&superblock))
    return -1;

  for (size_t i = 0; i < superblock.inode_count; i++) {
    if (!fs_read_inode(i, &inode))
      return -1;

    if (inode.type == FS_TYPE_FREE) {
      inode.type = type;
      inode.size = 0;

      memset(inode.blocks, 0, sizeof(inode.blocks));

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
```

Block allocation and inode allocation are similar, but they are separate. This
function does not allocate any data blocks, it creates inode metadata, blocks
for the data get allocated later when needed in future functions like `fs_write_file`.
All this function does is iterate through the inode table, read each inode, find
one whose type is `FS_TYPE_FREE`, initialize it, write it back, decrease `free_inodes`, and
return the inode number. If a free inode is not found, -1 gets returned instead.

### `fs_free inode`

```c
bool fs_free_inode(uint32_t inode_num) {
  fs_superblock_t superblock;
  fs_inode_t inode;
  bool failure = false;

  if (!fs_read_inode(inode_num, &inode))
    return false;

  for (size_t i = 0; i < FS_INODE_MAX_BLOCKS; i++) {
    if (inode.blocks[i] != 0) {
      //need a variable to represent failure, this is becuase we don't wanna return halfway though writing
      //on a failure because this will corrupt the filesystem.
      if (!fs_free_block(inode.blocks[i]))
        failure = true;

      inode.blocks[i] = 0;
    }
  }

  if (failure)
    return false;

  inode.type = FS_TYPE_FREE;
  inode.size = 0;

  if (!fs_write_inode(inode_num, &inode))
    return false;

  if (!fs_read_superblock(&superblock))
    return false;

  superblock.free_inodes++;

  if (!fs_write_superblock(&superblock))
    return false;

  return true;
}
```

Unlike the previous, this function will affect data blocks, as when an inode becomes free,
we also want to go through ever block referenced by it and free it. The function attempts
to free all the inode's blocks instead of returning the first failure. This reduces the chance
of leaving the inode in a partially cleaned-up state.


### `fs_find_directory_entry`

```c
int32_t fs_find_directory_entry(uint32_t directory_inode_num, const char* name) {
  fs_inode_t directory;
  uint8_t buffer[FS_BLOCK_SIZE];

  if (!fs_read_inode(directory_inode_num, &directory))
    return -1;

  uint32_t entries_per_block = FS_BLOCK_SIZE / sizeof(fs_directory_entry_t);

  for (size_t i = 0; i < FS_INODE_MAX_BLOCKS; i++) {
    if (directory.blocks[i] == 0)
      break;

    if (!fs_read_block(directory.blocks[i], buffer))
      return -1;

    fs_directory_entry_t* entries = (fs_directory_entry_t*)buffer;

    for (uint32_t j = 0; j < entries_per_block; j++) {
      if (entries[j].inode == 0) 
        continue;

      if (strcmp(name, entries[j].name) == 0)
        return entries[j].inode;
    }

  }
  return -1;
}
```

The purpose function is to find an entry in a directory that matches the name that
we pass to the function. A directory inode's blocks contain `fs_directory_entry_t`
structures, the function reads each directory block and examines every entry. 
`inode == 0` means the entry is unused, if the name matches, it returns
the associated inode number, -1 means the name wasn't found. Inode 0 is also the inode
for the root directory, but we would never have to search for the root directory by a name as it
would never be contained within a parent directory.

### `fs_add_directory_entry`

```c
bool fs_add_directory_entry(uint32_t directory_inode_num, uint32_t inode_num, const char* name) {
  fs_inode_t directory;
  uint8_t buffer[FS_BLOCK_SIZE];

  if (!fs_read_inode(directory_inode_num, &directory))
    return false;

  uint32_t entries_per_block = FS_BLOCK_SIZE / sizeof(fs_directory_entry_t);
   
  for (size_t i = 0; i < FS_INODE_MAX_BLOCKS; i++) {
    //no block is assigned so allocate a block
    if (directory.blocks[i] == 0) {
      int32_t block_num = fs_alloc_block();

      if (block_num < 0)
        return false;

      directory.blocks[i] = block_num;

      memset(buffer, 0, FS_BLOCK_SIZE);

      if (!fs_write_block(block_num, buffer)) {
        fs_free_block(block_num);
        return false;
      }
    }
    
    if (!fs_read_block(directory.blocks[i], buffer))
      return false;

    fs_directory_entry_t* entries = (fs_directory_entry_t*)buffer;

    //search for free entry indicated by the .inode, if not found, will go to next iteration of 
    //block loop, if free, allocate it accordingly and then return true
    for (size_t j = 0; j < entries_per_block; j++) {
      if (entries[j].inode == 0) {
        entries[j].inode = inode_num;

        memset(entries[j].name, 0, FS_FILENAME_LENGTH);

        strncpy(entries[j].name, name, FS_FILENAME_LENGTH - 1);

        if (!fs_write_block(directory.blocks[i], buffer))
          return false;

        directory.size += sizeof(fs_directory_entry_t);

        if (!fs_write_inode(directory_inode_num, &directory))
          return false;

        return true;
      }
    }
  }
  return false;
}
```

Opposite of the finding function, if there is no directory block, one 
is allocated using `fs_alloc_block()`. Again, here `inode == 0` means free
but also the root directory, we would never add the root directory to another
directory, so this does not matter. The function stores the inode number and
filename in a new directory entry, then it writes the block back and increases 
the directory's size.

### `fs_remove_directory_entry`

```c
bool fs_remove_directory_entry(uint32_t directory_inode_num, const char* name) {
  fs_inode_t directory;
  uint8_t buffer[FS_BLOCK_SIZE];

  if(!fs_read_inode(directory_inode_num, &directory))
    return false;

  uint32_t entries_per_block = FS_BLOCK_SIZE / sizeof(fs_directory_entry_t);

  for (size_t i = 0; i < FS_INODE_MAX_BLOCKS; i++) {
    if (directory.blocks[i] == 0)
      break;

    if (!fs_read_block(directory.blocks[i], buffer))
      return false;

    fs_directory_entry_t* entries = (fs_directory_entry_t*)buffer;

    for (uint32_t j = 0; j < entries_per_block; j++) {

      if (entries[j].inode == 0)
        continue;

      if (strcmp(name, entries[j].name) == 0) {
        //entry to remove found
        entries[j].inode = 0;
        
        if (!fs_write_block(directory.blocks[i], buffer))
          return false;

        directory.size -= sizeof(fs_directory_entry_t);

        if (!fs_write_inode(directory_inode_num, &directory))
          return false;

        return true;
      }
    }
  }
  return false;
}
```

Find the entry by name, set it's inode number to 0,
write the directory's block back, decrease the directory's size.
This function does not delete the inode or its data blacks, it only removes
the name to inode mapping from the directory. The actual inode and its blocks
are freed by future functions that we create like `fs_delete_file()`.

### `fs_create_file`

```c
int32_t fs_create_file(uint32_t directory_inode_num, const char* name) {
  if (fs_find_directory_entry(directory_inode_num, name) >= 0)
    return -1;

  int32_t file_inode = fs_alloc_inode(FS_TYPE_FILE);

  if (file_inode < 0)
    return -1;

  if (!fs_add_directory_entry(directory_inode_num, file_inode, name)) {
    fs_free_inode(file_inode);
    return -1;
  }

  return file_inode;
}
```

This function marks the point where we have all our low level filesystem operations, so we can now 
start combining them all together to make higher level operations. A newly
created empty file has an inode, but it doesn't need a data block until something is
written to it, so, for this function, we simply allocate an inode to the file
and add it to the directory that the file belongs to. Any errors result in
`file_inode` being freed.

### `fs_create_directory`

```c
int32_t fs_create_directory(uint32_t parent_inode_num, const char* name) {
  if (fs_find_directory_entry(parent_inode_num, name) >= 0)
    return -1;

  int32_t dir_inode_num = fs_alloc_inode(FS_TYPE_DIRECTORY);

  if (dir_inode_num < 0)
    return -1;

  int32_t block_num = fs_alloc_block();

  if (block_num < 0) {
    fs_free_inode(dir_inode_num);
    return -1;
  }

  fs_inode_t dir_inode;

  if (!fs_read_inode(dir_inode_num, &dir_inode))
    goto fail;

  memset(dir_inode.blocks, 0, sizeof(dir_inode.blocks));

  dir_inode.blocks[0] = block_num;
  dir_inode.size = 0;

  uint8_t buffer[FS_BLOCK_SIZE];

  memset(buffer, 0, FS_BLOCK_SIZE);

  if (!fs_write_block(block_num, buffer))
    goto fail;

  if (!fs_write_inode(dir_inode_num, &dir_inode))
    goto fail;

  if (!fs_add_directory_entry(parent_inode_num, dir_inode_num, name))
    goto fail;

  return dir_inode_num;

fail:
  fs_free_block(block_num);
  fs_free_inode(dir_inode_num);
  return -1;
}
```

The sequence goes as follows:
1.  Check the name and ensure that the directory doesn't already exist.
2.  Allocate an inode for the directory.
3.  Allocate a data block.
4.  Read the allocated inode from the filesystem
5.  Clear all the blocks it links to
6.  Give the directory its block
7.  Clear the directory's block
8.  Add directory to the parent.

A directory needs a block to store its directory entries, whereas previously
an empty regular file does not need a data block yet.
The `fail:` path exists because several resources may have already been allocated
by the time something fales, so if something goes wrong, we attempt to clean 
absolutely everything.

### `fs_read_file`

```c
int32_t fs_read_file(uint32_t file_inode_num, void* read_buffer, uint32_t size, uint32_t offset) {
  fs_inode_t file_inode;
  uint8_t buffer[FS_BLOCK_SIZE];

  if (!fs_read_inode(file_inode_num, &file_inode))
    return -1;

  if (offset >= file_inode.size)
    return 0; //EOF should return 0 bytes instead of an error

  if (offset + size > file_inode.size)
    size = file_inode.size - offset;

  uint32_t bytes_read = 0;

  uint8_t* destination = (uint8_t*)read_buffer;

  while (bytes_read < size) {
    uint32_t position = offset + bytes_read;

    uint32_t block_index = position / FS_BLOCK_SIZE;
    uint32_t block_offset = position % FS_BLOCK_SIZE;

    if (block_index >= FS_INODE_MAX_BLOCKS)
      break;

    if (!fs_read_block(file_inode.blocks[block_index], buffer))
        return -1;

    uint32_t bytes = FS_BLOCK_SIZE - block_offset;

    if (bytes > size - bytes_read)
      bytes = size - bytes_read;

    memcpy(destination + bytes_read, buffer + block_offset, bytes);

    bytes_read += bytes;
  }

  return bytes_read;
}
```

The function receives two variables that may seem unfamiliar, these being
the size and the offset. `offset` is the offset of bytes into the file's blocks that 
we want to read from and `size` is the number of bytes that we want to read.
`offset` is allowed to span over the file's multiple blocks as we can calculate
the block number and byte offset into it.
The function also has some EOF behaviour:
-   If the offset is beyond the file size (not the size we read), return 0.
-   If the requested data extends beyond the file, reduce the requested
    size to the remaining file data.

### `fs_write_file`

```c
int32_t fs_write_file(uint32_t inode_num, const void* write_buffer, uint32_t size, uint32_t offset) {
  fs_inode_t file_inode;
  uint8_t buffer[FS_BLOCK_SIZE];

  if (!fs_read_inode(inode_num, &file_inode))
    return -1;

  const uint8_t* source = (const uint8_t*)write_buffer;
  uint32_t bytes_written = 0;

  while (bytes_written < size) {
    uint32_t position = offset + bytes_written;

    uint32_t block_index = position / FS_BLOCK_SIZE;
    uint32_t block_offset = position % FS_BLOCK_SIZE;

    if (block_index >= FS_INODE_MAX_BLOCKS)
      break;

    //block not allocated, so allocate one and make it free, if else just read block.
    if (file_inode.blocks[block_index] == 0) {
      int32_t block = fs_alloc_block();

      if (block < 0)
        return -1;

      file_inode.blocks[block_index] = block;

      memset(buffer, 0, FS_BLOCK_SIZE);
    } else {
      if (!fs_read_block(file_inode.blocks[block_index], buffer))
          return -1;
    }

    uint32_t bytes = FS_BLOCK_SIZE - block_offset;

    if (bytes > size - bytes_written)
      bytes = size - bytes_written;

    memcpy(buffer + block_offset, source + bytes_written, bytes);

    if (!fs_write_block(file_inode.blocks[block_index], buffer))
      return -1;

    bytes_written += bytes;
  }

  if (offset + bytes_written > file_inode.size)
    file_inode.size = offset + bytes_written;

  if (!fs_write_inode(inode_num, &file_inode))
    return -1;

  return bytes_written;
}
```

Like the last function the `block_index` and `block_offset` are both calculated
from the file position. `if (file_inode.blocks[block_index] == 0)` means 
there is no physical block that has been assigned there, so a block
must get allocated for us to write to it.
A block that we write to must get read before writing to it, this matters
because writing to a part of an existing block must preserve the rest of 
the block.

`bytes = FS_BLOCK_SIZE - block_offset` is a required calculation because the function
writes as much as possible into the current block, then the loop moves onto the 
next block if more data remains. The file size is then updated if necessary, and 
the updated inode is written back.

### `fs_delete_file`

```c
bool fs_delete_file(uint32_t directory_inode, const char* name) {
  int32_t inode_num = fs_find_directory_entry(directory_inode, name);

  if (inode_num < 0)
    return false;

  if (!fs_remove_directory_entry(directory_inode, name))
    return false;

  if (!fs_free_inode(inode_num))
    return false;

  return true;
}
```

This ties our previous functions together. Deleting a file requires dealing
with three different pieces of metadata: The directory entry, the inode, and its
data blocks. This function coordinates all our lower-level functions that clean 
our filesystem.

This function can also delete directories, this is because the lower level functions we use
don't really care about the type of inode we delete.

## Conclusion

That's all for the core of the filesystem, you can technically just have this and use this
to manage all files, but I'd like for a cleaner way of interfacing with the filesystem.
This is what we will be making next.
