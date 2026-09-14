/* ============================================================
 * Chapter 30 -- Processes
 *
 * Create two kernel processes and watch the timer switch between
 * them and the main kernel process, without any of the three
 * cooperating or even knowing about each other.
 * ============================================================ */

#include "kernel/drivers/vga_text.h"
#include "kernel/interrupts.h"
#include "kernel/drivers/timer.h"
#include "kernel/drivers/keyboard.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "memory/heap.h"
#include "tasks/procman.h"
#include "tasks/scheduler.h"
#include "filesystem/fs_manager.h"

#include <stdint.h>

vga_text terminal;

/* Counters the three processes bump independently. They are globals
 * because all kernel processes share the kernel address space. */
static volatile uint32_t count_a = 0;
static volatile uint32_t count_b = 0;

/* A kernel process is just a function. Two rules:
 *
 *   1. It must NEVER return. There is no return address on its
 *      stack to go back to, so returning jumps into garbage.
 *      End it with an infinite loop, or terminate it explicitly.
 *
 *   2. It cannot call the user-space helpers like sleep() or exit():
 *      those are system calls, and it is already inside the kernel.
 */
static void task_a(void) {
    for (;;) count_a++;
}

static void task_b(void) {
    for (;;) count_b++;
}

void kernel_main(void)
{
    vga_text_init(&terminal);
    idt_init();
    init_pmm();
    init_vmm();
    init_heap();
    init_procman();      /* must come before create_process */
    timer_init(100);     /* the scheduler runs off this */
    keyboard_init();
    init_tss();
    fs_manager_init();

    vga_text_set_color(&terminal, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_text_clear(&terminal);
    vga_text_writeline(&terminal, "-- three kernel processes sharing one CPU --");
    vga_text_writeline(&terminal, "");

    process_t* a = create_process(task_a, PROCESS_KERNEL);
    process_t* b = create_process(task_b, PROCESS_KERNEL);

    vga_text_write(&terminal, "created pid ");
    vga_text_write_dec(&terminal, a->pid);
    vga_text_write(&terminal, " and pid ");
    vga_text_write_dec(&terminal, b->pid);
    vga_text_writeline(&terminal, "");
    vga_text_writeline(&terminal, "");

    /* This loop is the third process (pid 1, created by init_procman).
     * Nothing here yields: the timer interrupt takes the CPU away
     * every time slice whether this code likes it or not. That is
     * what preemptive means. */
    for (uint32_t round = 0; round < 6; round++) {
        timer_wait_ms(50);          /* counts ticks, so ~500 ms */

        vga_text_write(&terminal, "round ");
        vga_text_write_dec(&terminal, round + 1);
        vga_text_write(&terminal, ":  task_a ran to ");
        vga_text_write_hex(&terminal, count_a);
        vga_text_write(&terminal, "   task_b ran to ");
        vga_text_write_hex(&terminal, count_b);
        vga_text_writeline(&terminal, "");
    }

    vga_text_writeline(&terminal, "");
    vga_text_set_color(&terminal, VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_text_writeline(&terminal,
        "both counters advanced: all three processes got the CPU");
    vga_text_set_color(&terminal, VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    for (;;);
}
