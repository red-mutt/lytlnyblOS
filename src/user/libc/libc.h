#ifndef LIBC_H
#define LIBC_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../../kernel/mappings.h"

typedef struct heap_header{
  size_t size;
  bool free;
  struct heap_header* next;
} heap_header_t;

void init_heap();
void split(heap_header_t *block, size_t size);
void* malloc(size_t bytes);
void merge(heap_header_t* block);
void free(void* ptr);


#endif
