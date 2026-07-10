#ifndef PPM_H
#define PPM_H

#include <stdint.h>

#define FRAME_SIZE 4096
#define BITMAP_BASE 0x100000

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t attributes;
} __attribute__((packed)) memory_map_entry_t;

void init_pmm();

void bitmap_set_frame(uint32_t frame_index);
void bitmap_clear_frame(uint32_t frame_index);

#endif
