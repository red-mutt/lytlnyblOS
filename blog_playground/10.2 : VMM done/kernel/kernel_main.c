#include "vga_text.h"
#include "interrupts.h"
#include "timer.h"
#include "keyboard.h"
#include "../memory/pmm.h"
#include "../memory/vmm.h"

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

    for (;;);
}
