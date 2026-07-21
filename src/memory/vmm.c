#include "vmm.h"
#include "../kernel/vga_text.h"
#include "pmm.h"

extern vga_text terminal;

page_directory_t* kernel_directory;
page_directory_t* current_directory;


void init_vmm() {
    kernel_directory = alloc_frame();
    page_table_t* tbl_ptr = alloc_frame();

    memset(kernel_directory, 0, 4096);
    memset(tbl_ptr, 0, 4096);
    
    //gives every page up to the limit an entry in the table.
    for (uint32_t i = 0; i < 1024; i++) {
        (*tbl_ptr)[i] = (i * 0x1000) | PAGE_PRESENT | PAGE_WRITABLE;
    }

    (*kernel_directory)[0] = ((uintptr_t)tbl_ptr) | PAGE_PRESENT | PAGE_WRITABLE;    

    current_directory = kernel_directory;
    set_cr3((uintptr_t)kernel_directory);
    set_cr0();
}

page_table_t* create_page_table(uint32_t directory_index, uint32_t flags) {
    page_table_t* new_table = alloc_frame();
    memset(new_table, 0, 4096);
    (*current_directory)[directory_index] = (uintptr_t)new_table | flags;
    return new_table;
}

void map_page(uintptr_t virtual_address, uintptr_t physical_address, uint32_t flags) {
    uint16_t dir_index = (virtual_address >> 22);
    uint16_t table_index = (virtual_address >> 12); 

    uint32_t dir_entry = (*current_directory)[dir_index];
    page_table_t* selected_table;
    if (!(dir_entry & PAGE_PRESENT)) {
        selected_table = create_page_table(dir_index, flags);
    } else {
        selected_table = (page_table_t*)(dir_entry & 0xFFFFF000);
    }

    (*selected_table)[table_index] = physical_address | flags;
    flush_tlb_page(virtual_address);
}

void unmap_page(uintptr_t virtual_address) {
    uint16_t dir_index = (virtual_address >> 22);
    uint16_t table_index = (virtual_address >> 12);  

    uint32_t dir_entry = (*current_directory)[dir_index];
    page_table_t* selected_table = (page_table_t*)(dir_entry & FFFFF000);
    (*selected_table)[table_index] &= ~(PAGE_PRESENT);
    flush_tlb_page(virtual_address);
}





