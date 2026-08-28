#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>
#include <stddef.h>

#define SYSCALL_EXIT 0x01
#define SYSCALL_GETPID 0x02
#define SYSCALL_YIELD 0x03
#define SYSCALL_SLEEP 0x04
#define SYSCALL_WRITE 0x05
#define SYSCALL_READ 0x06
#define SYSCALL_SBRK 0x07
#define SYSCALL_FSOPS 0x08

#define FS_LS 0
#define FS_MKDIR 1
#define FS_TOUCH 2
#define FS_RM 3

extern uint32_t syscall(uint32_t CODE, uint32_t a, uint32_t b, uint32_t c);

void exit(void);
uint32_t get_pid(void);
void yield();
void sleep(uint32_t ticks);
int write(int fd, void *buff, size_t count);
int read(int fd, void* buff, size_t count);
void* sbrk (intptr_t increment);
void fs_ops(uint8_t operation, char* path);


#endif
