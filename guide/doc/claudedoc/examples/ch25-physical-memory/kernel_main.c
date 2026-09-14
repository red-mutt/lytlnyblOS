/* ============================================================
 * Chapter 25 -- The physical memory manager
 *
 * Asking for and returning 4 KB frames of real RAM.
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
#include "kernel/kernel_utils.h"

#include <stdint.h>

vga_text terminal;

static void show(const char* label, uint32_t value) {
    vga_text_write(&terminal, label);
    vga_text_write_hex(&terminal, value);
    vga_text_writeline(&terminal, "");
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
    vga_text_writeline(&terminal, "-- allocating physical frames --");
    vga_text_writeline(&terminal, "");

    /* 1. Ask for three frames. Each is 4096 bytes of real RAM, and
     *    the address returned is PHYSICAL. */
    void* a = alloc_frame();
    void* b = alloc_frame();
    void* c = alloc_frame();

    show("frame a: ", (uint32_t)a);
    show("frame b: ", (uint32_t)b);
    show("frame c: ", (uint32_t)c);
    vga_text_writeline(&terminal, "");

    /* 2. alloc_frame returns NULL when memory runs out. Always check:
     *    map_page() would happily map address 0 otherwise. */
    if (a == NULL || b == NULL || c == NULL) {
        vga_text_writeline(&terminal, "out of physical memory");
        for (;;);
    }

    /* 3. The frames are usable directly ONLY because they are below
     *    4 MB, which is the part of RAM the kernel identity maps.
     *    Anything above that must be mapped before it can be touched. */
    memset(a, 0xAA, FRAME_SIZE);
    show("a[0] after memset: ", ((uint8_t*)a)[0]);
    show("a[4095] after memset: ", ((uint8_t*)a)[FRAME_SIZE - 1]);
    vga_text_writeline(&terminal, "");

    /* 4. Give one back, then ask again. First fit means the allocator
     *    hands the same frame straight back. */
    free_frame(b);
    void* d = alloc_frame();
    show("freed b, then allocated d: ", (uint32_t)d);

    if (d == b) {
        vga_text_set_color(&terminal, VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_text_writeline(&terminal, "d == b : the freed frame was reused");
    } else {
        vga_text_set_color(&terminal, VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_text_writeline(&terminal, "d != b : unexpected");
    }
    vga_text_set_color(&terminal, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_text_writeline(&terminal, "");

    /* 5. Clean up. Note free_frame validates nothing at all: a wrong
     *    address here silently corrupts the allocator's bitmap. */
    free_frame(a);
    free_frame(c);
    free_frame(d);
    vga_text_writeline(&terminal, "all frames returned");

    for (;;);
}
