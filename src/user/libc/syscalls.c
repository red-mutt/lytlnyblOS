#include "syscalls.h"

void exit() {
  syscall(SYSCALL_EXIT, 0, 0, 0);
} //get warning here due to function name maybe, ignore it

uint32_t get_pid() {
  return syscall(SYSCALL_GETPID, 0, 0, 0);
}

void yield() {
  syscall(SYSCALL_YIELD, 0, 0, 0);
}

void sleep(uint32_t ticks) {
  syscall(SYSCALL_SLEEP, ticks, 0, 0);
}

int write(int fd, void *buff, size_t count) {
  return syscall(SYSCALL_WRITE, fd, (uint32_t)buff, count);
}


int read(int fd, void* buff, size_t count) {
  return syscall(SYSCALL_READ, fd, (uint32_t)buff, count);
}

void* sbrk (intptr_t increment) {
  return (void*)syscall(SYSCALL_SBRK, increment, 0, 0);
}

