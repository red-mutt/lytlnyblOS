/* ============================================================
 * Chapter 45 -- reproducing the reboot loop
 *
 * The case study traced a reboot loop back to terminal.width being
 * zeroed by a stray allocation. Rather than wait for that to happen
 * by accident, this file zeroes it on purpose, so the failure is
 * reproducible and you can practise the diagnosis.
 *
 *   make run     watch it reboot over and over
 *   make trace   boot with -no-reboot -d int,cpu_reset and see why
 *
 * `make trace` is the useful one. It stops at the triple fault
 * instead of rebooting, and prints the last exceptions -- which is
 * exactly how the real bug was found.
 *
 * WHAT TO LOOK FOR, and it is the whole lesson:
 *
 *   a long run of `v=00` (exception 0, divide by zero) at the SAME
 *   instruction address, with SP dropping by exactly 0x9C (156
 *   bytes) each time.
 *
 * A repeating vector at a fixed IP with a monotonically falling SP
 * is the signature of runaway recursion, and it is what identified
 * the original bug. After it, the corruption spreads -- you will
 * also see `v=06` (invalid opcode) as execution wanders into
 * damaged memory -- and it ends in `v=08` (double fault) and a
 * triple fault, which is the reset you see as a reboot loop.
 *
 * The addresses will not match the ones quoted in the chapter: the
 * real fault appeared later in boot, on a different stack. The
 * recursion signature is the part that is identical, and the part
 * worth recognising.
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
#include "kernel/mappings.h"

#include <stdint.h>

vga_text terminal;

/* The failure is triggered from a kernel PROCESS, not from
 * kernel_main, and that detail matters.
 *
 * kernel_main runs on the boot stack, which lives in .bss inside the
 * identity-mapped low 4 MB. Overflowing it quietly corrupts other
 * kernel data and the recursion just keeps going.
 *
 * A process created with create_process gets its stack from kmalloc,
 * so it sits at the bottom of the kernel heap at 0xC0000000 -- and
 * there is nothing mapped below that. Overflowing THAT stack walks
 * straight off the edge of the address space, which is what turns
 * the recursion into a page fault, a double fault and a reset.
 *
 * That is exactly the stack the original bug ran on. */
static void doomed_task(void)
{
    terminal.width = 0;

    /* Any print at all now divides by zero:
     *
     *   vga_text_write divides by terminal.width  -> exception 0
     *   isr_handler prints "Divide By Zero"       -> calls write
     *   vga_text_write divides by terminal.width  -> exception 0
     *
     * Each round pushes about 156 bytes and never returns. */
    vga_text_writeline(&terminal, "here we go");

    for (;;);
}

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
    vga_text_writeline(&terminal, "starting the doomed process...");

    /* Zeroing terminal.width is what the stray allocation did by
     * accident: a memcpy of the shell binary landed on the kernel's
     * globals, because the last partial page of the kernel was left
     * unprotected by a truncating division in pmm.c.
     *
     * The process below does it deliberately, then prints. */
    create_process(doomed_task, PROCESS_KERNEL);

    /* Once the timer schedules it, the recursion begins. After
     * roughly 107 rounds the 16 KB kernel stack is gone, the next
     * push lands below 0xC0000000 where nothing is mapped, the fault
     * handler faults on the same broken stack, and the processor
     * gives up and resets. */
    for (;;);
}
