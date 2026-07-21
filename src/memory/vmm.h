#ifndef VMM_H
#define VMM_H

#define PAGE_PRESENT (1 << 0)
#define PAGE_WRITABLE (1 << 1)
#define PAGE_USER (1 << 2)

#include <stdint.h>
#include "../kernel/interrupts.h"

typedef uint32_t page_directory_t[1024];
typedef uint32_t page_table_t[1024];

void init_vmm();

extern void set_cr3(uintptr_t dir_ptr);
extern void set_cr0();

//calc dir index, calc table index, find/create page table, insert page dir
void map_page(
    uintptr_t virtual_address,
    uintptr_t physical_address,
    uint32_t flags
);

void unmap_page(uintptr_t virtual_address);
uintptr_t get_physical_address(uintptr_t virtual_address);

extern void flush_tlb(void);
extern void flush_tlb_page(uintptr_t virtual_address);

// Page table management

page_table_t* create_page_table(uint32_t directory_index, uint32_t flags);
void destroy_page_table(uint32_t directory_index);


// Address space management

page_directory_t* create_address_space(void);
void switch_address_space(page_directory_t* directory);
void destroy_address_space(page_directory_t* directory);


// Page fault handling

void page_fault_handler(registers_t* registers);



#endif
