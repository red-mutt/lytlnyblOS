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
uint32_t fs_fd_to_inode(int32_t fd);

int32_t fs_open(const char* path);
int32_t fs_close(int fd);
int32_t fs_read(int fd, void* buffer, uint32_t size);
int32_t fs_write(int fd, void* buffer, uint32_t size);

bool fs_mkdir(const char* path);
void fs_ls(const char* path);
bool fs_touch(const char* path);
bool fs_rm(const char* path);

#endif
