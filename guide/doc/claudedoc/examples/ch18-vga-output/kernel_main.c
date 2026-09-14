/* ============================================================
 * Chapter 18 -- The text driver
 *
 * Everything you can do with the VGA text driver, in one screen.
 *
 * This file replaces kernel/kernel_main.c. The initialisation
 * sequence at the top is the real one and must stay: every later
 * subsystem depends on it. Only the demonstration below it changes.
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
#include "kernel/drivers/ata.h"

#include <stdint.h>

/* The one terminal instance. Every other file reaches it with
 * `extern vga_text terminal;`, so it must be defined here. */
vga_text terminal;

void kernel_main(void)
{
    /* --- the standard bring-up, unchanged --- */
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

    /* --- the example starts here --- */

    vga_text_set_color(&terminal, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_text_clear(&terminal);

    /* 1. Plain text. write() leaves the cursor where it is;
     *    writeline() moves to the start of the next row. */
    vga_text_write(&terminal, "write() does not ");
    vga_text_write(&terminal, "start a new line.");
    vga_text_writeline(&terminal, "");
    vga_text_writeline(&terminal, "writeline() does.");
    vga_text_writeline(&terminal, "");

    /* 2. Numbers. There is no printf in the kernel, so decimal and
     *    hexadecimal have a function each. */
    vga_text_write(&terminal, "decimal:     ");
    vga_text_write_dec(&terminal, 1234567);
    vga_text_writeline(&terminal, "");

    vga_text_write(&terminal, "hexadecimal: ");
    vga_text_write_hex(&terminal, 0xDEADBEEF);
    vga_text_writeline(&terminal, "");
    vga_text_writeline(&terminal, "");

    /* 3. Colour: foreground first, background second. It applies to
     *    everything written from here on, until changed again.
     *    Note the enum has no VGA_COLOR_YELLOW: the colour everyone
     *    calls yellow is VGA_COLOR_LIGHT_BROWN. */
    vga_text_set_color(&terminal, VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_text_writeline(&terminal, "light green on black");

    vga_text_set_color(&terminal, VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY);
    vga_text_writeline(&terminal, "black on light grey");

    vga_text_set_color(&terminal, VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
    vga_text_writeline(&terminal, "light brown, which is really yellow");

    vga_text_set_color(&terminal, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_text_writeline(&terminal, "");

    /* 4. Writing a single character at an exact cell, without moving
     *    the cursor the rest of the driver is using. */
    vga_text_put_entry_at(&terminal, '#',
                          VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK, 20, 40);
    vga_text_writeline(&terminal, "there is now a red # at row 20, column 40");

    /* 5. Reading a character back off the screen. The screen is just
     *    memory, so it can be read as easily as written. */
    size_t saved_row = terminal.row;
    vga_text_set_cursor(&terminal, 20, 40);
    char c = vga_text_getchar(&terminal);
    vga_text_set_cursor(&terminal, saved_row, 0);

    vga_text_write(&terminal, "getchar() at (20,40) read back: '");
    vga_text_putchar(&terminal, c);
    /* putchar does not advance the cursor, so step over it by hand */
    vga_text_set_cursor(&terminal, saved_row, 34);
    vga_text_writeline(&terminal, "'");

    for (;;);
}
