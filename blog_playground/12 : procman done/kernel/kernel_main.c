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

    kprocess_t* p1 = create_kprocess(NULL);
    kprocess_t* p2 = create_kprocess(NULL);

    vga_text_write(&terminal, "PIDs: ");
    vga_text_write_hex(&terminal, p1->pid);
    vga_text_write(&terminal, " ");
    vga_text_write_hex(&terminal, p2->pid);
    vga_text_writeline(&terminal, "");

    if (find_process_by_pid(p1->pid) == p1 && find_process_by_pid(p2->pid) == p2) {
        vga_text_writeline(&terminal, "CREATE/LOOKUP OK");
    }

    destroy_kprocess(p1);

    if (find_process_by_pid(p1->pid) == NULL) {
        vga_text_writeline(&terminal, "DESTROY OK");
    }

    for (;;);
}
