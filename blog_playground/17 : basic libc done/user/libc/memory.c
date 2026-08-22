#include "memory.h"

uint8_t* memory;
heap_header_t* heap_start_head;
uintptr_t heap_end;

void init_heap() {
  heap_end = USER_HEAP_START + 4096;
  heap_start_head = (void*)USER_HEAP_START;
  heap_start_head->size = 4096 - sizeof(heap_header_t);
  heap_start_head->free = true;
  heap_start_head->next = NULL;
}

void split_block(heap_header_t* block, size_t requested_size) {
  heap_header_t* new = (void*)((void*)block + requested_size + sizeof(heap_header_t));
  new->size = (block->size)-requested_size-sizeof(heap_header_t);
  new->free = true;
  new->next = block->next;

  block->size = requested_size;
  block->free = false;
  block->next = new;
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

void expand_heap(size_t requested_size) {
  uintptr_t original_end = heap_end;
  heap_end = (uintptr_t)sbrk(requested_size);

  heap_header_t* traversal_head = heap_start_head;
  while (traversal_head) {
    if (!traversal_head->next && !traversal_head->free) {
      heap_header_t* new_header = (heap_header_t*)original_end;

      new_header->free = true;
      new_header->next = NULL;
      new_header->size = requested_size - sizeof(heap_header_t);

      traversal_head->next = new_header;
      break;
    } else if(!traversal_head->next && traversal_head->free) {
      traversal_head->size += requested_size;
      break;
    }
    traversal_head = traversal_head->next;
  }
}

void *malloc(size_t bytes) {
  void *result;
  heap_header_t *curr;

  if (!(heap_start_head->size))init_heap();  

  heap_header_t* block = find_free_block(bytes);
  if (block) {
    if (block->size != bytes) split_block(block, bytes);
    block->free = false;
    result = (void*)((uintptr_t)block + sizeof(heap_header_t));
  } else {
    expand_heap(bytes);
    return malloc(bytes);
  }
  return result;
}

void merge_blocks(heap_header_t* block) {
  if (!block->next || !block->next->free) return;
  heap_header_t* block_to_merge = block->next;
  
  block->size += block_to_merge->size + sizeof(heap_header_t);
  block->next = block_to_merge->next;
  return merge_blocks(block);
}

heap_header_t* get_header(void* ptr) {
  return (heap_header_t*)((uintptr_t)ptr - sizeof(heap_header_t));
}

void free(void* ptr) {
  if (!ptr) return;
  heap_header_t* block_to_free = get_header(ptr);
  block_to_free->free = true;
  merge_blocks(block_to_free);
}

void* memcpy(void* dest, const void* src, size_t n) {
  uint8_t* d = (uint8_t*)dest;
  const uint8_t* s = (const uint8_t*)src;

  for (size_t i = 0; i < n; i++) {
    d[i] = s[i];
  }

  return dest;
}

void* memmove(void* dest, const void* src, size_t n) {
  uint8_t* d = (uint8_t*)dest;
  const uint8_t* s = (const uint8_t*)src;

  if (d == s || n == 0) return dest;

  if (d < s) {
    //safe to copy won't overwrite the source
    for(size_t i = 0; i < n; i++) {
      d[i] = s[i];
    }
  } else {
    // copy backwards so source isn't overwritten
    for (size_t i = n; i > 0; i--) {
      d[i - 1] = s[i - 1];
    }
  } 
  return dest;
}

void* memset(void* ptr, uint8_t c, size_t n) {
  uint8_t* p = ptr;
  while (n--) {
    *p++ = (c);
  }
  return ptr;
}

int memcmp(void* ptr1, void* ptr2, size_t n) {
  size_t i;
  uint8_t* p1 = (uint8_t*)ptr1;
  uint8_t* p2 = (uint8_t*)ptr2;
  int compare_status = 0;

  if (ptr1 == ptr2) return compare_status;

  while (n > 0) {
    if (*p1 != *p2) {
      compare_status = (*p1 > *p2) ? 1 : -1;
      break;
    }
    n--;
    p1++;
    p2++;
  }
  return compare_status;
}

void* calloc(size_t n, size_t size) {
  size_t total = n * size;
  void *ptr = malloc(total);

  if (ptr) memset(ptr,0,total);
  return ptr;
}

void* realloc(void* ptr, size_t new_size) {
  void *new_ptr = malloc(new_size);
  if (new_ptr == NULL) return NULL;

  memcpy(new_ptr, ptr, get_header(ptr)->size);
  free(ptr);
  return new_ptr;
}
