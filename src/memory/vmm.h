#ifndef VMM_H
#define VMM_H

#define PAGE_PRESENT (1 << 0)
#define PAGE_WRITABLE (1 << 1)
#define PAGE_USER (1 << 2)

//page fault definitions
#define PRESENT_FAULT (1 << 0)
#define WRITE_FAULT (1 << 1)
#define USER_FAULT (1 << 2)
#define RESERVED_FAULT (1 << 3)
#define INSTRUCTION_FETCH_FAULT (1 << 4)

#define PHYS_MAP_BASE 0xC0000000

#include <stdint.h>
#include "../kernel/interrupts.h"

typedef uint32_t page_directory_t[1024];
typedef uint32_t page_table_t[1024];

void init_vmm();

extern void set_cr3(uintptr_t dir_ptr);
extern void set_cr0();
extern  uint32_t get_cr2();

//calc dir index, calc table index, find/create page table, insert page dir
void map_page(
    page_directory_t* directory,
    uintptr_t virtual_address,
    uintptr_t physical_address,
    uint32_t flags
);

void unmap_page(page_directory_t* directory, uintptr_t virtual_address);
uintptr_t get_physical_address(page_directory_t* directory, uintptr_t virtual_address);

extern void flush_tlb(void);
extern void flush_tlb_page(uintptr_t virtual_address);

// Page table management

page_table_t* create_page_table(page_directory_t* directory, uint32_t directory_index, uint32_t flags);

// Page fault handling
void page_fault_handler(registers_t* registers);

extern page_directory_t* kernel_directory;
extern page_directory_t* current_directory;

#endif
