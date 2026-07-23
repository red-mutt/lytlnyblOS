#include "heap.h"
#include "vmm.h"
#include "pmm.h"

heap_block_t* heap_head;

void init_heap() {
    void* physical_start = alloc_frame();
    map_page(HEAP_START, (uintptr_t)physical_start, PAGE_PRESENT | PAGE_WRITABLE);
    heap_header_t* first_header = (heap_header_t*)HEAP_START;

    first_header->size = 4096 - sizeof(heap_header_t); //4096 is page size 
    first_header->free = true;
    first_header->next = NULL;

    heap_head = first_header;
}
