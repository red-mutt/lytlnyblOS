# The Filesystem

This will be a long and grueling task. My file systems code is around 700 lines,
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

Our implementation for the file system will have two header files. One called 
`fs_layout.h` that contains the definitions and structures for the filesystem, 
and one called `fs.h` that contains the functions for the file system.
Why do we do this? This is because later, in the programs section, we will make 
a program called MKFS (make file system) which will populate our filesystem with
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
define locations in the file system. `START_BLOCK` states that the whole filesystem
should start at the seventieth block in the disk image, this gives ample space for the
filesystem without overwriting the kernel. The next 3 locations are all then relative to this
start block.

After our locations, there is the definition for the max file name that a directory can
contain. Next, there is our
inode types: `FREE` an inode to be allocated, `DIRECTORY` for directories and `FILE` for
files.

Next we have some more definitions of the maximums for our file system. The maximum
number of blocks that an inode can point to, the maximum number of blocks that an inode can point to
and finally the total number of inodes for the whole file system.

>**_NOTE:_** When writing any programs, you should be mindful of the 
`FS_INODE_MAX_BLOCKS` value. `10 x 512 bytes` is 5120 bytes, so this will become
the greatest size for any programs or files you eventually store in the filesystem 
unless you change this.

`FS_ROOT_INODE` is the inode index for the root directory that would be made during
initialization as you cannot store anything without a root directory.
`FS_MAGIC` is our file system's signature. This is a value we check to ensure
that the file system we write to and read from is actually the one we have programmed.

### The structures

Our first structure is the superblock, this will be read from a lot in 
our implementation's code. Most of its data mirrors the previous definitions.
The superblock contains: 

-   The magic number (signature) for our file system
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


