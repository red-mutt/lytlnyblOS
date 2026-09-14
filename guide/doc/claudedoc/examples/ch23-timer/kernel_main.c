/* ============================================================
 * Chapter 23 -- The timer
 *
 * Counting ticks, measuring how long something took, and seeing
 * what changing the PIT frequency actually changes.
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

static void label(const char* text, uint32_t value, const char* unit) {
    vga_text_write(&terminal, text);
    if (value == 0) vga_text_write(&terminal, "0");   /* write_dec bug */
    else            vga_text_write_dec(&terminal, value);
    vga_text_writeline(&terminal, unit);
}

/* Something slow enough to measure. */
static volatile uint32_t sink;
static void burn(uint32_t rounds) {
    for (uint32_t i = 0; i < rounds; i++)
        for (uint32_t j = 0; j < 100000; j++)
            sink += j;
}

void kernel_main(void)
{
    vga_text_init(&terminal);
    idt_init();
    init_pmm();
    init_vmm();
    init_heap();
    init_procman();
    timer_init(100);        /* 100 Hz: one tick every 10 ms */
    keyboard_init();
    init_tss();
    fs_manager_init();

    vga_text_set_color(&terminal, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_text_clear(&terminal);
    vga_text_writeline(&terminal, "-- the timer --");
    vga_text_writeline(&terminal, "");

    /* 1. The tick counter is just a global the handler increments.
     *    It is volatile because an interrupt changes it behind the
     *    compiler's back. */
    label("ticks since boot: ", (uint32_t)timer_get_ticks(), "");

    /* 2. Timing something. Subtract two readings. At 100 Hz each
     *    tick is 10 ms, so multiply by 10 for milliseconds. */
    uint32_t start = (uint32_t)timer_get_ticks();
    burn(40);
    uint32_t elapsed = (uint32_t)timer_get_ticks() - start;

    label("that work took ", elapsed, " ticks");
    label("which is about ", elapsed * 10, " ms");
    vga_text_writeline(&terminal, "");

    /* 3. timer_wait_ms is misnamed: it counts TICKS, not
     *    milliseconds. Asking for 100 waits 100 ticks = 1 second. */
    vga_text_writeline(&terminal, "calling timer_wait_ms(100)...");
    uint32_t t0 = (uint32_t)timer_get_ticks();
    timer_wait_ms(100);
    uint32_t waited = (uint32_t)timer_get_ticks() - t0;
    label("it actually waited ", waited, " ticks, i.e. 1 second, not 100 ms");
    vga_text_writeline(&terminal, "");

    /* 4. Reprogramming the frequency. At 1000 Hz a tick is 1 ms, so
     *    the same work reports ten times as many ticks. The work did
     *    not change: the ruler did. */
    vga_text_writeline(&terminal, "switching the PIT to 1000 Hz...");
    timer_init(1000);

    start = (uint32_t)timer_get_ticks();
    burn(40);
    elapsed = (uint32_t)timer_get_ticks() - start;
    label("the same work now reports ", elapsed, " ticks");
    label("which at 1 ms per tick is ", elapsed, " ms");

    vga_text_writeline(&terminal, "");
    vga_text_set_color(&terminal, VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
    vga_text_writeline(&terminal,
        "note the scheduler's time slice is counted in ticks too,");
    vga_text_writeline(&terminal,
        "so raising the frequency also makes it switch processes more often");
    vga_text_set_color(&terminal, VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    for (;;);
}
