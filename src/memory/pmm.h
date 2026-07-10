#ifndef PPM_H
#define PPM_H

#include <stdint.h>

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t attributes;
} __attribute__((packed)) memory_map_entry_t;

void init_pmm();

#endif
