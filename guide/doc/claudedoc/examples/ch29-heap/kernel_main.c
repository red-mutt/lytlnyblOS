/* ============================================================
 * Chapter 29 -- The kernel heap
 *
 * kmalloc and kfree, and what the block list does underneath.
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
#include "kernel/mappings.h"

#include <stdint.h>

vga_text terminal;

static void show(const char* label, uint32_t value) {
    vga_text_write(&terminal, label);
    vga_text_write_hex(&terminal, value);
    vga_text_writeline(&terminal, "");
}

/* Walk the block list and report it. The header sits immediately
 * before the memory it describes, so get_header() steps backwards. */
static void dump_heap(const char* when) {
    vga_text_set_color(&terminal, VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_text_write(&terminal, "heap ");
    vga_text_writeline(&terminal, when);
    vga_text_set_color(&terminal, VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    heap_header_t* b = (heap_header_t*)HEAP_START;
    int n = 0;
    while (b && n < 8) {
        vga_text_write(&terminal, "  block at ");
        vga_text_write_hex(&terminal, (uint32_t)b);
        vga_text_write(&terminal, "  size ");
        vga_text_write_dec(&terminal, b->size);
        vga_text_writeline(&terminal, b->free ? "  FREE" : "  used");
        b = b->next;
        n++;
    }
}

void kernel_main(void)
{
    vga_text_init(&terminal);
    idt_init();
    init_pmm();
    init_vmm();
    init_heap();
    /* The rest of the bring-up is not optional even in an example
     * that only uses the heap: idt_init() enables interrupts, so the
     * timer will fire and call the scheduler, and the scheduler walks
     * the process list. Skip init_procman() and that list is NULL. */
    init_procman();
    timer_init(100);
    keyboard_init();
    init_tss();
    fs_manager_init();

    vga_text_set_color(&terminal, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_text_clear(&terminal);

    /* 1. One allocation splits the single big free block in two. */
    uint32_t* a = kmalloc(64);
    uint32_t* b = kmalloc(128);
    show("a = ", (uint32_t)a);
    show("b = ", (uint32_t)b);

    /* The gap between them is 64 bytes of data plus one header. */
    show("b - a = ", (uint32_t)b - (uint32_t)a);
    vga_text_writeline(&terminal, "");

    /* 2. The memory really is usable. */
    for (int i = 0; i < 16; i++) a[i] = i * i;
    show("a[15] = ", a[15]);
    vga_text_writeline(&terminal, "");

    dump_heap("after two allocations");
    vga_text_writeline(&terminal, "");

    /* 3. Freeing a marks it free and merges it forward with the
     *    block after it only if that one is free too. Here b is
     *    still in use, so no merge happens. */
    kfree(a);
    dump_heap("after kfree(a)");
    vga_text_writeline(&terminal, "");

    /* 4. Now free b as well. a and b are adjacent and both free, but
     *    merge_blocks only walks FORWARD, so freeing b merges b with
     *    whatever follows, and a stays a separate small free block. */
    kfree(b);
    dump_heap("after kfree(b) too");

    for (;;);
}
