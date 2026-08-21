#include "libc.h"

uint8_t* memory;
heap_header_t* heap_start_head;

void init_heap() {
  heap_start_head = (void*)USER_HEAP_START;
  heap_start_head->size = 4096 - sizeof(heap_header_t);
  heap_start_head->free = true;
  heap_start_head->next = NULL;
}
