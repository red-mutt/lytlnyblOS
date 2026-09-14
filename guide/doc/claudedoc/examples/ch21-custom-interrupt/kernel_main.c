/* ============================================================
 * Chapter 21 -- triggering our own interrupt vector
 *
 * patch.py adds vector 0x30 to the IDT. This file fires it and
 * shows that the handler can read and modify the interrupted
 * code's registers through the registers_t pointer.
 * ============================================================ */

#include "kernel/drivers/vga_text.h"
#include "kernel/interrupts.h"
#include "kernel/drivers/timer.h"
#include "kernel/drivers/keyboard.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "memory/heap.h"
#include "tasks/procman.h"
#include "filesystem/fs_manager.h"

#include <stdint.h>

vga_text terminal;

void kernel_main(void)
{
    vga_text_init(&terminal);
    idt_init();
    init_pmm();
    init_vmm();
    init_heap();
    init_procman();
    timer_init(100);
    keyboard_init();
    init_tss();
    fs_manager_init();

    vga_text_set_color(&terminal, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_text_clear(&terminal);
    vga_text_writeline(&terminal, "-- a custom interrupt vector --");
    vga_text_writeline(&terminal, "");

    uint32_t before = 0x11112222;
    uint32_t after  = 0;

    vga_text_write(&terminal, "before: EBX will be set to ");
    vga_text_write_hex(&terminal, before);
    vga_text_writeline(&terminal, "");

    /* Put a known value in EBX, raise the interrupt, and read EBX
     * back afterwards. The handler overwrites it through regs->ebx,
     * which is the saved copy that iret restores. */
    asm volatile(
        "mov %1, %%ebx\n\t"
        "int $0x30\n\t"
        "mov %%ebx, %0"
        : "=r"(after)
        : "r"(before)
        : "ebx");

    vga_text_write(&terminal, "after:  EBX came back as ");
    vga_text_write_hex(&terminal, after);
    vga_text_writeline(&terminal, "");
    vga_text_writeline(&terminal, "");

    if (after == 0x5A5A5A5A) {
        vga_text_set_color(&terminal, VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_text_writeline(&terminal,
            "the handler changed a register of the code it interrupted");
        vga_text_writeline(&terminal,
            "that is exactly how a system call returns its result");
    } else {
        vga_text_set_color(&terminal, VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_text_writeline(&terminal, "unexpected value");
    }
    vga_text_set_color(&terminal, VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    for (;;);
}
