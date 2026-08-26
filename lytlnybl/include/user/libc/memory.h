#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../../kernel/mappings.h"
#include "syscalls.h"

typedef struct heap_header{
  size_t size;
  bool free;
  struct heap_header* next;
} heap_header_t;

void* malloc(size_t bytes);
void free(void* ptr);
int memcmp(void* ptr1, void* ptr2, size_t n);
void* memcpy(void* dest, const void* src, size_t n);
void* memmove(void* dest, const void* src, size_t bytes);
void* memset(void* ptr, uint8_t c, size_t n);
void* calloc(size_t n, size_t size);
void* realloc(void* ptr, size_t new_size);


#endif
