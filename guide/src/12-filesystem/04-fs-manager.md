# The filesystem manager

## The header

As we said before in the introduction, the file system manager implements these functions
shown in the header file:

```c
#ifndef FS_MANAGER_H
#define FS_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#define FS_MAX_OPEN_FILES 16
#define FS_FD_OFFSET 3

typedef struct {
    uint32_t inode;
    uint32_t offset;
    bool used;
} fs_file_t;

void fs_manager_init(void);

int32_t fs_resolve_path(const char* path);

int32_t fs_open(const char* path);
int32_t fs_close(int fd);
int32_t fs_read(int fd, void* buffer, uint32_t size);
int32_t fs_write(int fd, void* buffer, uint32_t size);

bool fs_mkdir(const char* path);
void fs_ls(const char* path);
bool fs_touch(const char* path);
bool fs_rm(const char* path);

#endif
```

The `fs_file_t` type stores metadata for files that are currently open, they 
will get stored in an array and will to track the offset into a file when 
reading and writing to it. `FS_MAX_OPEN_FILES` will be the size of this array.
The array will be defined as such:

```c
static fs_file_t open_files[FS_MAX_OPEN_FILES];
```

`FS_FD_OFFSET` is the offset that gets used to convert a "file descriptor" to an index.
`fd` 0 is standard input, 1 is standard output, and 2 is standard error.
Every file descriptor beyond 2 is an index into this array that represents open files.

`fs_resolve_path` gets used to convert a path from a string to an inode number.
A path may look like: `/bin/user_test`. Where the first slash represents the root directory,
`bin` is a directory within the root directory, and `user_test` is a program within `bin`.

Our operating system's path directories will only be absolute, this means that there is no such thing
as `cd` or a current working directory. Any time we want to access something it will be from root.

`fs_fd_to_inode` accesses the array, applies the offset, and returns the inode number associated with the `fd`.
This function will get used externally.

Then we have the next four functions which are our primitive operations for interfacing with files:
-   `fs_open` takes a path to a file, opens it (by adding it to the open files array), and returns the `fd` 
    number.
-   `fs_close` takes a file descriptor and closes it.
-   `fs_read` reads from a file using a size and adds the bytes read from in `fs_file_t`.
-   `fs_write` same as the prior but with writing.

The next four functions are all self-explanatory, let's get into writing the
implementation file.

## The implementation

```c
#include "filesystem/fs_manager.h"
#include "filesystem/fs.h"
#include "kernel/drivers/ata.h"
#include "kernel/drivers/vga_text.h"

extern vga_text terminal;
static fs_file_t open_files[FS_MAX_OPEN_FILES];

void fs_manager_init() {
  init_ata();

  for (int i = 0; i < FS_MAX_OPEN_FILES; i++) {
    open_files[i].used = false;
  }
}

int32_t fs_resolve_path(const char* path) {
  if (!path || path[0] != '/')
    return -1;

  uint32_t current_inode = FS_ROOT_INODE;
  uint32_t i = 1;

  while (path[i] != '\0') {
    char name[FS_FILENAME_LENGTH];
    uint32_t name_length = 0;

    while (path[i] == '/')
      i++;

    if (path[i] == '\0')
      break;

    while (path[i] != '/' && path[i] != '\0') {
      if (name_length >= FS_FILENAME_LENGTH - 1)
        return -1;

      name[name_length] = path[i];
      name_length++;
      i++;
    }

    name[name_length] = '\0';

    int32_t inode = fs_find_directory_entry(current_inode, name);

    if (inode < 0)
      return -1;

    current_inode = inode;
  }
  return current_inode;
}

int32_t fs_open(const char* path) {
  int32_t inode = fs_resolve_path(path); 

  if (inode < 0)
    return -1;

  for (int i = 0; i < FS_MAX_OPEN_FILES; i++) {
    if (!open_files[i].used) {
      open_files[i].used = true;
      open_files[i].inode = inode;
      open_files[i].offset = 0;

      return i + FS_FD_OFFSET;
    }
  }
  return -1;
}

int32_t fs_close(int fd) {
  fd -= FS_FD_OFFSET;
  if (fd < 0 || fd >= FS_MAX_OPEN_FILES)
    return -1;

  if (!open_files[fd].used)
    return -1;

  open_files[fd].used = false;
  open_files[fd].inode = 0;
  open_files[fd].offset = 0;

  return 0;
}

int32_t fs_read(int fd, void* buffer, uint32_t size) {
  fd -= FS_FD_OFFSET;
  if (fd < 0 || fd >= FS_MAX_OPEN_FILES)
    return -1;

  if (!open_files[fd].used)
    return -1;

  int32_t bytes_read = fs_read_file(open_files[fd].inode,
      buffer,
      size,
      open_files[fd].offset
  );
  
  if (bytes_read < 0)
    return -1;

  open_files[fd].offset += bytes_read;

  return bytes_read;
}

int32_t fs_write(int fd, void* buffer, uint32_t size) {
  fd -= FS_FD_OFFSET;
  if (fd < 0 || fd >= FS_MAX_OPEN_FILES)
    return -1;

  if (!open_files[fd].used)
    return -1;

  int32_t bytes_read = fs_write_file(open_files[fd].inode,
      buffer,
      size,
      open_files[fd].offset
  );
  
  if (bytes_read < 0)
    return -1;

  open_files[fd].offset += bytes_read;

  return bytes_read;
}

static bool split_path(const char* path, char* parent, char* name) {
  int length = 0;

  while (path[length] != '\0')
    length++;

  if (length == 0)
    return false;

  int last_slash = -1;
  for (int i = 0; i< length; i++) {
    if (path[i] == '/')
      last_slash = i;
  }

  int name_length = length - last_slash - 1;

  if (name_length <= 0 || name_length >= FS_FILENAME_LENGTH)
    return false;

  for (int i = 0; i < name_length; i++)
    name[i] = path[last_slash + 1 + i];

  name[name_length] = '\0';

  if (last_slash == 0) {
    parent[0] = '/';
    parent[1] = '\0';
  } else {
    for (int i = 0; i < last_slash; i++)
      parent[i] = path[i];

    parent[last_slash] = '\0';
  }

  return true;
}

bool fs_mkdir(const char* path) {
  char parent[256];
  char name[FS_FILENAME_LENGTH];

  if (!split_path(path, parent, name))
    return false;

  int32_t parent_inode = fs_resolve_path(parent);

  if (parent_inode < 0)
    return false;

  fs_inode_t inode;

  if (!fs_read_inode(parent_inode, &inode))
    return false;

  if (inode.type != FS_TYPE_DIRECTORY)
    return false;

  return fs_create_directory(parent_inode, name) >= 0;
}


bool fs_touch(const char* path) {
  char parent[256];
  char name[FS_FILENAME_LENGTH];

  if (!split_path(path, parent, name))
    return false;

  int32_t parent_inode = fs_resolve_path(parent);

  if (parent_inode < 0)
    return false;

  fs_inode_t inode;

  if (!fs_read_inode(parent_inode, &inode))
    return false;

  if (inode.type != FS_TYPE_DIRECTORY)
    return false;

  return fs_create_file(parent_inode, name) >= 0;
}

bool fs_rm(const char* path) {
  char parent[256];
  char name[FS_FILENAME_LENGTH];

  if (!split_path(path, parent, name))
    return false;

  int32_t parent_inode = fs_resolve_path(parent);

  if (parent_inode < 0)
    return false;

  fs_inode_t inode;

  if (!fs_read_inode(parent_inode, &inode))
    return false;

  if (inode.type != FS_TYPE_DIRECTORY)
    return false;

  return fs_delete_file(parent_inode, name);
}

void fs_ls(const char* path) {
  int32_t directory_inode = fs_resolve_path(path);

  if (directory_inode < 0) 
    return;

  fs_inode_t directory;

  if (!fs_read_inode(directory_inode, &directory))
    return;

  if (directory.type != FS_TYPE_DIRECTORY)
    return;

  uint8_t buffer[FS_BLOCK_SIZE];

  uint32_t entries_per_block = FS_BLOCK_SIZE / sizeof(fs_directory_entry_t);

  for (int i = 0; i < FS_INODE_MAX_BLOCKS; i++) {
    if (directory.blocks[i] == 0)
      break;

    if (!fs_read_block(directory.blocks[i], buffer))
      return;

    fs_directory_entry_t* entries = (fs_directory_entry_t*)buffer;

    for (uint32_t j = 0; j < entries_per_block; j++) {
      if (entries[j].inode == 0)
        continue;

      fs_inode_t entry_inode_obj;

      if (!fs_read_inode(entries[j].inode, &entry_inode_obj))
        return;

      if (entry_inode_obj.type == FS_TYPE_DIRECTORY) {
        vga_text_set_color(&terminal, VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_RED);
      } else {
        vga_text_set_color(&terminal, VGA_COLOR_LIGHT_CYAN, VGA_COLOR_RED);
      }
      vga_text_write(&terminal, entries[j].name);
      vga_text_write(&terminal, " ");
      vga_text_set_color(&terminal, VGA_COLOR_WHITE, VGA_COLOR_RED);
    }
  }
  vga_text_writeline(&terminal, "");
}

uint32_t fs_fd_to_inode(int32_t fd) {
  fd -= FS_FD_OFFSET;
  return open_files[fd].inode; 
}
```
