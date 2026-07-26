#include "heap.h"
#include "vmm.h"
#include "pmm.h"

heap_header_t* heap_start_head;
extern uintptr_t heap_end;

void init_heap() {
    void* physical_start = alloc_frame();
    map_page(HEAP_START, (uintptr_t)physical_start, PAGE_PRESENT | PAGE_WRITABLE);
    heap_header_t* first_header = (heap_header_t*)HEAP_START;

    first_header->size = 4096 - sizeof(heap_header_t); //4096 is page size 
    first_header->free = true;
    first_header->next = NULL;

    heap_start_head = first_header;
    heap_end = HEAP_START + 0x1000;
}

heap_header_t* find_free_block(size_t requested_size) {
    heap_header_t* current_head = heap_start_head;
    while (current_head) {
        if (current_head->free && current_head->size >= requested_size) {
            return current_head;
        }
        current_head = current_head->next;
    }
    return NULL;
}

void split_block(heap_header_t* block, size_t requested_size) {
    size_t space_remaining = (block->size - requested_size);
    if (space_remaining < sizeof(heap_header_t) + sizeof(uint8_t)) {
        return;
        //don't make block, not big enough
    };

    uintptr_t origin_block_end = (uintptr_t)(block) + block->size + sizeof(heap_header_t);
    uintptr_t new_block_start = (uintptr_t)(block) + requested_size + sizeof(heap_header_t);
    heap_header_t* new_block = (heap_header_t*)(new_block_start);
    
    new_block->size = origin_block_end - new_block_start - sizeof(heap_header_t);
    new_block->free = true;

    heap_header_t* temp_next = block->next;
    new_block->next = temp_next;
    block->next = new_block;
    
    block->size = requested_size;
}

void expand_heap(size_t required_size) {
    uint32_t required_pages = (required_size + 4096 - 1) / 4096;
    for (size_t i = 0; i < required_pages; i++) {
        void* new_page_physical_start = alloc_frame();
        map_page(heap_end, (uintptr_t)new_page_physical_start, PAGE_PRESENT | PAGE_WRITABLE);

        heap_header_t* traversal_head = heap_start_head;
        while (traversal_head) {
            if (!traversal_head->next && !traversal_head->free) {
                heap_header_t* new_frame_header = (heap_header_t*)heap_end;
        
                new_frame_header->free = true;
                new_frame_header->next = NULL;
                new_frame_header->size = 4096 - sizeof(heap_header_t);

                traversal_head->next = new_frame_header;
                break;
            } else if (!traversal_head->next && traversal_head->free) {
                traversal_head->size += 4096;
                break; //will just be empty space for the previous free head        
            }
            traversal_head = traversal_head->next;
        }
        heap_end += 0x1000;
    } 
}

void* kmalloc(size_t requested_size) {
    if (!requested_size) return NULL;

    heap_header_t* block = find_free_block(requested_size);
    if (block) {
        split_block(block, requested_size);
        block->free = false;
        return (void*)((uintptr_t)block + sizeof(heap_header_t)); //return free memory not the head
    } else {
        expand_heap(requested_size);
        return kmalloc(requested_size);
    }
}

heap_header_t* get_header(void* ptr) {
    return (heap_header_t*)((uintptr_t)ptr - sizeof(heap_header_t));
}

void merge_blocks(heap_header_t* block) {
    if (!block->next || !block->next->free) return;
    heap_header_t* block_to_merge = block->next;
    
    block->size += block_to_merge->size + sizeof(heap_header_t);
    block->next = block_to_merge->next;
    return merge_blocks(block); //do to next
}

void kfree(void* ptr) {
    if (!ptr) return;
    heap_header_t* block_to_free = get_header(ptr);
    block_to_free->free = true;
    merge_blocks(block_to_free);
}
