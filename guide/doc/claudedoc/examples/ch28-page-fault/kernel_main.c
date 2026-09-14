/* ============================================================
 * Chapter 28 -- Page faults
 *
 * Touch an address that is not mapped and read what the processor
 * and the handler tell you about it.
 *
 * The system deliberately stops afterwards: a fault raised by the
 * kernel itself has no safe recovery, so isr_handler halts in an
 * infinite loop. A frozen screen with a report on it is much easier
 * to debug than a machine that keeps running while corrupt.
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

/* 0x50000000 is not mapped in any directory this system builds. */
#define NOWHERE 0x50000000u

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

    vga_text_write(&terminal, "about to write to ");
    vga_text_write_hex(&terminal, NOWHERE);
    vga_text_writeline(&terminal, ", which is not mapped");
    vga_text_writeline(&terminal, "");

    /* Predict the report before reading it:
     *   Address  should be 0x50000000   (the processor puts it in CR2)
     *   Reason   Page not present       (error code bit 0 clear)
     *   Access   Write                  (bit 1 set)
     *   Mode     Kernel                 (bit 2 clear: we are in ring 0)
     * so the error code should be 0x2. */

    *(volatile uint32_t*)NOWHERE = 1;

    /* Never reached. */
    vga_text_writeline(&terminal, "if you can read this, something is wrong");
    for (;;);
}
