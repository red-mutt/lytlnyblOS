/* ============================================================
 * Chapter 27 -- Paging in practice
 *
 * Mapping a page, proving the mapping is real, aliasing two
 * virtual addresses onto one frame, translating by hand, unmapping,
 * and building a fresh address space.
 *
 * We deliberately work at 0x40000000 (1 GB). That address is not
 * used by anything: it is above the identity-mapped low 4 MB, below
 * the kernel heap at 0xC0000000, and outside every user region.
 * Mapping something inside the low 4 MB instead would overwrite an
 * existing identity mapping and break the kernel underneath us.
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

#define SCRATCH_A 0x40000000u    /* first virtual address we will map  */
#define SCRATCH_B 0x40001000u    /* second one, onto the same frame    */

static void show(const char* label, uint32_t value) {
    vga_text_write(&terminal, label);
    vga_text_write_hex(&terminal, value);
    vga_text_writeline(&terminal, "");
}

/* vga_text_write_dec has a bug for the value 0: it writes the '0'
 * with putchar but forgets to advance the cursor, so whatever is
 * printed next lands on top of it and the zero disappears. Every
 * other value goes through a loop that does advance the cursor.
 * This wrapper works around it so the indices below are readable. */
static void write_dec(uint32_t value) {
    if (value == 0) {
        vga_text_write(&terminal, "0");
        return;
    }
    vga_text_write_dec(&terminal, value);
}

static void ok(const char* msg) {
    vga_text_set_color(&terminal, VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_text_writeline(&terminal, msg);
    vga_text_set_color(&terminal, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

static void bad(const char* msg) {
    vga_text_set_color(&terminal, VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_text_writeline(&terminal, msg);
    vga_text_set_color(&terminal, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
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

    /* ---------- 1. map one page ---------- */

    void* frame = alloc_frame();
    if (!frame) { bad("out of memory"); for (;;); }

    map_page(kernel_directory, SCRATCH_A, (uintptr_t)frame,
             PAGE_PRESENT | PAGE_WRITABLE);

    show("physical frame:  ", (uint32_t)frame);
    show("mapped at:       ", SCRATCH_A);

    /* map_page already flushed the TLB for this address because we
     * modified the directory that is currently loaded in CR3. */

    /* ---------- 2. prove it is the same memory ---------- */

    /* Write through the new virtual address... */
    *(volatile uint32_t*)SCRATCH_A = 0xC0FFEE01;

    /* ...and read it back through the physical address, which works
     * because the frame is below 4 MB and therefore identity mapped. */
    uint32_t seen = *(volatile uint32_t*)frame;
    show("wrote 0xC0FFEE01 via virtual, read via physical: ", seen);

    if (seen == 0xC0FFEE01) ok("same memory, two names");
    else                    bad("mismatch");
    vga_text_writeline(&terminal, "");

    /* ---------- 3. translate by hand, and with the helper ---------- */

    uint32_t dir_index   =  SCRATCH_A >> 22;
    uint32_t table_index = (SCRATCH_A >> 12) & 0x3FF;
    uint32_t offset      =  SCRATCH_A & 0xFFF;

    vga_text_write(&terminal, "directory index: ");
    write_dec(dir_index);
    vga_text_write(&terminal, "   table index: ");
    write_dec(table_index);
    vga_text_write(&terminal, "   offset: ");
    write_dec(offset);
    vga_text_writeline(&terminal, "");

    show("get_physical_address says: ",
         get_physical_address(kernel_directory, SCRATCH_A));
    vga_text_writeline(&terminal, "");

    /* ---------- 4. two virtual addresses, one frame ---------- */

    map_page(kernel_directory, SCRATCH_B, (uintptr_t)frame,
             PAGE_PRESENT | PAGE_WRITABLE);

    *(volatile uint32_t*)SCRATCH_B = 0xFEEDFACE;
    uint32_t via_a = *(volatile uint32_t*)SCRATCH_A;
    show("wrote via B, read via A: ", via_a);

    if (via_a == 0xFEEDFACE) ok("aliasing works: one frame, two addresses");
    else                     bad("aliasing failed");
    vga_text_writeline(&terminal, "");

    /* ---------- 5. unmap ---------- */

    unmap_page(kernel_directory, SCRATCH_B);
    vga_text_writeline(&terminal, "unmapped B (A still works)");
    show("read via A after unmapping B: ", *(volatile uint32_t*)SCRATCH_A);

    /* Careful: unmap_page only clears the present bit. It does NOT
     * return the frame to the physical allocator, and it does NOT
     * zero the entry, so get_physical_address still reports an
     * address for B even though touching B would now fault. */
    show("get_physical_address(B) after unmap: ",
         get_physical_address(kernel_directory, SCRATCH_B));
    vga_text_writeline(&terminal, "the helper does not check the present bit");
    vga_text_writeline(&terminal, "");

    /* ---------- 6. a whole new address space ---------- */

    page_directory_t* space = (page_directory_t*)alloc_frame();
    memset(space, 0, 4096);

    /* Copy the kernel's mappings in, or interrupts would fault the
     * moment this directory were loaded into CR3. */
    for (uint32_t i = 0; i < 1024; i++)
        (*space)[i] = (*kernel_directory)[i];

    /* Then add something private to it, user-accessible this time. */
    void* private_frame = alloc_frame();
    map_page(space, 0x00400000, (uintptr_t)private_frame,
             PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);

    show("new directory at:        ", (uint32_t)space);
    show("its entry for 0x400000:  ", (*space)[1]);
    show("kernel's entry for same: ", (*kernel_directory)[1]);
    vga_text_writeline(&terminal,
        "0x400000 exists in one space and not the other");

    /* We never load this directory into CR3, so nothing changes for
     * the running kernel. Switching to it is set_cr3(), and that is
     * exactly what context_switch() does for every process. */

    unmap_page(kernel_directory, SCRATCH_A);
    free_frame(frame);

    for (;;);
}
