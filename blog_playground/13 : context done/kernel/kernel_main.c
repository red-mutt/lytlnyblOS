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

void test_process() {
    vga_text_writeline(&terminal, "PROCESS RUNNING");
    old_process_pid = 2;
    new_process_pid = 1;
    context_switch_requested = true;
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
    timer_init(100);
    keyboard_init();
    init_pmm(); 
    init_vmm();
    init_heap();
    init_procman();

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

    kprocess_t* test_proc= create_kprocess(test_process);
    old_process_pid = 1;
    new_process_pid = test_proc->pid;
    context_switch_requested = true;
    timer_wait_ms(10);

    vga_text_writeline(&terminal, "back in main");
    

    for (;;);
}
