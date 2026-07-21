#ifndef VMM_H
#define VMM_H

#define PAGE_PRESENT (1 << 0)
#define PAGE_WRITABLE (1 << 1)
#define PAGE_USER (1 << 2)

#include <stdint.h>

typedef uint32_t page_directory_t[1024];
typedef uint32_t page_table_t[1024];

void init_vmm();

extern void set_cr3(uintptr_t dir_ptr);
extern void set_cr0();

#endif
