#ifndef VMM_H
#define VMM_H

#define PAGE_PRESENT (1 << 0)
#define PAGE_WRITABLE (1 << 1)
#define PAGE_USER (1 << 2)

typedef uint32_t page_directory_t[1024];
typedef uint32_t page_table_t[1024];

void vmm_init();

#endif
