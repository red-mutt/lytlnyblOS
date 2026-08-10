#include "vga_text.h"
#include "interrupts.h"
#include "timer.h"
#include "keyboard.h"
#include "../memory/pmm.h"
#include "../memory/vmm.h"
#include "../memory/heap.h"
#include "../tasks/procman.h"

#include <stdint.h>

vga_text terminal;
#define USER_COPY_VIRT 0xC0000000

void test_process() {
    vga_text_writeline(&terminal, "PROCESS RUNNING");
    for (;;);
}

void kernel_main(void)
{
    volatile char* vga = (volatile char*)0xB8000;
    
    //signal that we have reached C
    vga[0] = 'C';
    vga[1] = 0x02;

    vga_text_init(&terminal);
    vga_text_writeline(&terminal, "Welcome to the lytlnybl kernel in protected mode");

    idt_init();

    init_pmm();
    init_vmm();
    init_heap();
    init_procman();

    timer_init(100);
    keyboard_init();

    uint32_t* numbers = (uint32_t*)kmalloc(5 * sizeof(uint32_t));

    for (int i = 0; i < 5; i++) {
        numbers[i] = (i + 1) * 10; // Stores 10, 20, 30, 40, 50
    }

    vga_text_write(&terminal, "Values: ");
    for (int i = 0; i < 5; i++) {
        vga_text_write_dec(&terminal, numbers[i]);
        vga_text_write(&terminal, " ");
    }
    vga_text_writeline(&terminal, "");

    kfree(numbers);

    process_t* test_proc= create_kprocess(test_process);
    timer_wait_ms(10);

    vga_text_writeline(&terminal, "back in main");

    //userspace testing
    extern unsigned char _binary_user_test_bin_start[];
    extern unsigned char _binary_user_test_bin_end[];

    uint32_t user_test_size = 
        _binary_user_test_bin_end - _binary_user_test_bin_start;

    process_t* user_proc = create_uprocess((void*)0x400000);

    set_cr3((uintptr_t)user_proc->page_directory);

    //map vga so ring 3 can access
    map_page(
    0xB8000,
    0xB8000,
    PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER
    );

    void* code_frame = alloc_frame();

    map_page(
        0x00400000,
        (uintptr_t)code_frame,
        PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER
    );

    //copy user program to physical frame
    uintptr_t user_size = (uintptr_t)(_binary_user_test_bin_end - 
            _binary_user_test_bin_start);

    for (uintptr_t i = 0; i < user_size; i++) {
        ((uint8_t*)USER_COPY_VIRT)[i] = _binary_user_test_bin_start[i];
    }


    for (;;);
}
