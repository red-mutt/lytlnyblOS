#include "vmm.h"
#include "../kernel/vga_text.h"
#include "pmm.h"
#include "../kernel/interrupts.h"

extern vga_text terminal;

void init_vmm() {
    page_directory_t* dir_ptr = alloc_frame();
    page_table_t* tbl_ptr = alloc_frame();

    memset(dir_ptr, 0, 4096);
    memset(tbl_ptr, 0, 4096);
    
    //gives every page up to the limit an entry in the table.
    for (uint32_t i = 0; i < 1024; i++) {
        (*tbl_ptr)[i] = (i * 0x1000) | PAGE_PRESENT | PAGE_WRITABLE;
    }

    (*dir_ptr)[0] = ((uintptr_t)tbl_ptr) | PAGE_PRESENT | PAGE_WRITABLE;

    set_cr3((uintptr_t)dir_ptr);
    set_cr0();
}

