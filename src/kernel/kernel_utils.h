#ifndef KERNEL_UTILS_H
#define KERNEL_UTILS_H

#include <stddef.h>
#include <stdint.h>

void* memset(void* dest, uint8_t val, size_t len);
void* memcpy(void* dest, const void* src, size_t len);
int32_t strcmp(const char* a, const char* b);
char* strncpy(char* dest, const char* src, size_t n);

#endif
