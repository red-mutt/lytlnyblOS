#ifndef HEAP_H
#define HEAP_H

#define HEAP_START 0x00400000 //dir index 1, eveyrthing else 0

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct heap_header{
    size_t size;
    bool free;
    struct heap_header* next;
} heap_header_t;

void init_heap(void);

void* kmalloc(size_t requested_size);
void kfree(void* ptr);

void expand_heap(size_t required_size);

// helpers

heap_header_t* find_free_block(size_t requested_size);

void split_block(
    heap_header_t* block,
    size_t size
);

void merge_blocks(
    heap_header_t* block
);

heap_header_t* get_header(void* ptr);


#endif
