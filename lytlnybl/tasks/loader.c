#include "tasks/loader.h"
#include "filesystem/fs_manager.h"
#include "filesystem/fs.h"
#include "tasks/procman.h"
#include "memory/pmm.h"
#include "kernel/mappings.h"

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

  size_t write_i = 0;
  for(size_t i = 0; i < ((file_size + 4096) / 4096); ++i) {
    void* code_frame = alloc_frame(); 

    
    //write to frame
    for (; write_i < 4096 && write_i < file_size; write_i++) {
      ((uint8_t*)code_frame)[write_i - (i * 4096)] = ((uint8_t*)buffer)[(i * 4096) + write_i];
    }

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
