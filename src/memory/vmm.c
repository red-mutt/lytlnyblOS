#include "vmm.h"
#include "../kernel/vga_text.h"
#include "pmm.h"
#include "../kernel/interrupts.h"

extern vga_text terminal;

void vmm_init() {

    uint32_t* ptr = (uint32_t*)0x200000;

    *ptr = 0x12345678;

    page_directory_t* dir_ptr = alloc_frame();
    page_table_t* tbl_ptr = alloc_frame();

    memset(dir_ptr, 0, 4096);
    memset(tbl_ptr, 0, 4096);
    
    //gives every page up to the limit an entry in the table.
    for (uint32_t i = 0; i < 1024; i++) {
        *tbl_ptr[i] = (i * 0x1000) | PAGE_PRESENT | PAGE_WRITABLE;
    }

    *dir_ptr[0] = ((uintptr_t)tbl_ptr) | PAGE_PRESENT | PAGE_WRITABLE;

    __asm__ volatile ( 
        ".intel_syntax noprefix;\n"
        "mov eax, %0;\n"
        "mov cr3, eax;\n"
        :
        : "r" (dir_ptr)
        : "eax"
    );

    uint32_t temp_eax;
    __asm__ volatile ( 
        ".intel_syntax noprefix;\n"
        "mov eax, cr0\n"
        "or eax, ~0x80000000;\n"
        "mov cr0, eax;\n"
        : "=a" (temp_eax)
        : 
        : "memory"
    );


    uint32_t value = *(uint32_t*)0x200000;

    //test paging

}

