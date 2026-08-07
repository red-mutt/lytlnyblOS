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
    uint16_t table_index = (virtual_address >> 12 & 0x3FF); 

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
    uint16_t table_index = (virtual_address >> 12 & 0x3FF); 

    uint32_t dir_entry = (*current_directory)[dir_index];
    page_table_t* selected_table = (page_table_t*)(dir_entry & 0xFFFFF000);
    (*selected_table)[table_index] &= ~(PAGE_PRESENT);
    flush_tlb_page(virtual_address);
}

uintptr_t get_physical_address(uintptr_t virtual_address) {
    uint16_t dir_index = (virtual_address >> 22);
    uint16_t table_index = (virtual_address >> 12 & 0x3FF); 

    uint32_t dir_entry = (*current_directory)[dir_index];
    page_table_t* selected_table = (page_table_t*)(dir_entry & 0xFFFFF000);
    return (*selected_table)[table_index];
}

void page_fault_handler(registers_t* registers) {

    uintptr_t address = get_cr2();
    uint32_t error_code = registers->error_code;

    vga_text_writeline(&terminal, "PAGE FAULT");

    vga_text_write(&terminal, "Address: ");
    vga_text_write_hex(&terminal, address);
    vga_text_writeline(&terminal, "");

    vga_text_write(&terminal, "Error code: ");
    vga_text_write_hex(&terminal, error_code);
    vga_text_writeline(&terminal, "");


    if (error_code & PRESENT_FAULT) {
        vga_text_writeline(&terminal, "Reason: Protection violation");
    }
    else {
        vga_text_writeline(&terminal, "Reason: Page not present");
    }


    if (error_code & WRITE_FAULT) {
        vga_text_writeline(&terminal, "Access: Write");
    }
    else {
        vga_text_writeline(&terminal, "Access: Read");
    }


    if (error_code & USER_FAULT) {
        vga_text_writeline(&terminal, "Mode: User");
    }
    else {
        vga_text_writeline(&terminal, "Mode: Kernel");
    }


    if (error_code & RESERVED_FAULT) {
        vga_text_writeline(&terminal, "Reserved bit violation");
    }


    if (error_code & INSTRUCTION_FETCH_FAULT) {
        vga_text_writeline(&terminal, "Instruction fetch fault");
    }


    vga_text_write(&terminal, "Instruction address: ");
    vga_text_write_hex(&terminal, registers->eip);
    vga_text_writeline(&terminal, "");


    // Stop execution
    while (1) {
        __asm__ volatile("cli; hlt");
    }
}

page_directory_t* get_current_directory() {
    return current_directory;
}
