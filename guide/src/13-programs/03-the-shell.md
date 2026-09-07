# Part XIII: The Shell

## What do we need to do?

We now have a way to get `.bin` programs from the host operating system into the
custom one. Now, the next order of business is to get programs from the 
filesystem and load them into memory. This won't be an external program like
MKFS, and we are back to expanding the custom OS and kernel. We also have
a nice interface for reading from the filesystem, so the interface we are making,
called a loader, will be short and simple.

After the loader gets made, the next step is to write the shell as a user space program.
This will include adding new syscalls to the operating system, that access these kernel utilities:
-   `write` and `read` syscalls need to be modified to use file descriptors.
-   `open` and `close` syscalls need to be made for the filesystem.
-   A syscall needs to be made for general filesystem operations that take in a path,
    like: `ls`, `mkdir`, `touch`, `rm` and `run`.

For the shell, new data needs to be introduced into `process_t`. This is data for running
programs as when a user space program runs another one, it needs to wait until the program
it's running is finished before continuing. Basically, When the shell runs another program it must be 
blocked until the child program finishes execution.

There also need to be modifications to `keyboard.c`, whenever we press the enter key, we want
to stop reading from `stdin`, backspaces must also be handled to remove from whatever is in
the process's buffer, otherwise backspace will only be visual.

## Writing the loader

I will make a small header here for the loader:
```c
#ifndef LOADER_H
#define LOADER_H

#include "tasks/procman.h"

process_t *load_program(const char* path);

#endif
```

Then the implementation file is here:

```c
#include "tasks/loader.h"
#include "filesystem/fs_manager.h"
#include "filesystem/fs.h"
#include "tasks/procman.h"
#include "memory/pmm.h"
#include "kernel/mappings.h"
#include "kernel/kernel_utils.h"

process_t* load_program(const char* path) {
  int32_t fd = fs_open(path);
  uint32_t inode_num = fs_fd_to_inode(fd);

  fs_inode_t file_inode;

  if (!fs_read_inode(inode_num, &file_inode))
    return NULL;

  size_t file_size = file_inode.size;

  uint8_t buffer[file_size];
  if (fs_read(fd, buffer, file_size) == -1)
    return NULL;

  process_t* user_proc = create_process((void*)USER_CODE_BASE, PROCESS_USER);

  for(size_t i = 0; i < ((file_size + 4095) / 4096); ++i) {
    void* code_frame = alloc_frame(); 

    
    //write to frame
    size_t offset = i * 4096;
    size_t bytes = file_size - offset;
    if (bytes > 4096)
      bytes = 4096;
    memcpy(code_frame, buffer + offset, bytes);

    map_page(
      user_proc->page_directory,
      USER_CODE_BASE + (4096 * i),
      (uintptr_t)code_frame,
      PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER
    ); 
  }

  fs_close(fd);

  return user_proc;
}
```



-DEV NOTES-
-write about the program loader
-large MKFS PART
-changes to the keyboard for backspace, returning on enter, and returning on read
-for running, need to block a process

