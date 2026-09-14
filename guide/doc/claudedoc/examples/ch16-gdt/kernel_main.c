/* ============================================================
 * Chapter 16 -- the GDT, as the processor actually holds it
 *
 * The table was built in assembly before C existed. Here we ask
 * the processor for it with SGDT, walk the descriptors, and decode
 * the access byte of each one -- confirming from live data what
 * the chapter says about 0x9A, 0xFA, 0x92 and 0xF2.
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

/* The operand of LGDT/SGDT: a 16-bit limit then a 32-bit base. */
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdtr_t;

static void hex(const char* label, uint32_t v) {
    vga_text_write(&terminal, label);
    vga_text_write_hex(&terminal, v);
    vga_text_writeline(&terminal, "");
}

static void dec(uint32_t v) {
    if (v == 0) vga_text_write(&terminal, "0");
    else        vga_text_write_dec(&terminal, v);
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

    /* 1. Ask the processor where its GDT is. */
    gdtr_t gdtr;
    asm volatile("sgdt %0" : "=m"(gdtr));

    hex("GDT base:  ", gdtr.base);
    vga_text_write(&terminal, "GDT limit: ");
    dec(gdtr.limit);
    vga_text_write(&terminal, "  -> ");
    dec((gdtr.limit + 1) / 8);
    vga_text_writeline(&terminal, " descriptors");
    vga_text_writeline(&terminal, "");
    vga_text_writeline(&terminal,
        "the limit is the LAST VALID BYTE, so entries = (limit+1)/8");
    vga_text_writeline(&terminal, "");

    /* 2. Walk the descriptors. Each is 8 bytes; byte 5 is access. */
    uint8_t* gdt = (uint8_t*)gdtr.base;
    uint32_t count = (gdtr.limit + 1) / 8;

    vga_text_set_color(&terminal, VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_text_writeline(&terminal, "sel  access  P DPL S E  meaning");
    vga_text_set_color(&terminal, VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    for (uint32_t i = 0; i < count && i < 8; i++) {
        uint8_t access = gdt[i * 8 + 5];

        if (i == 0) vga_text_write(&terminal, "00");
        else        vga_text_write_hex(&terminal, i * 8);
        vga_text_write(&terminal, "   ");
        if (access == 0) vga_text_write(&terminal, "00");
        else             vga_text_write_hex(&terminal, access);
        vga_text_write(&terminal, "     ");

        dec((access >> 7) & 1);            /* P   */
        vga_text_write(&terminal, "  ");
        dec((access >> 5) & 3);            /* DPL */
        vga_text_write(&terminal, "  ");
        dec((access >> 4) & 1);            /* S   */
        vga_text_write(&terminal, " ");
        dec((access >> 3) & 1);            /* E   */
        vga_text_write(&terminal, "  ");

        /* Compare against the source values with the Accessed bit
         * masked off: the processor SETS bit 0 by itself the first
         * time a descriptor is loaded into a segment register. */
        /* For a system segment (S = 0) also mask bit 1, which ltr
         * sets to mark the TSS busy. */
        uint8_t base_access = (access & 0x10) ? (access & ~0x01)
                                              : (access & ~0x03);
        switch (base_access) {
        case 0x00: vga_text_writeline(&terminal, "null (required)");      break;
        case 0x9A: vga_text_writeline(&terminal, "kernel code, ring 0");  break;
        case 0x92: vga_text_writeline(&terminal, "kernel data, ring 0");  break;
        case 0xFA: vga_text_writeline(&terminal, "user code, RING 3");    break;
        case 0xF2: vga_text_writeline(&terminal, "user data, RING 3");    break;
        case 0x88: vga_text_writeline(&terminal, "the TSS");              break;
        default:   vga_text_writeline(&terminal, "?");                    break;
        }
    }
    vga_text_writeline(&terminal, "");

    /* The table in memory is not quite the table in the source. */
    vga_text_set_color(&terminal, VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
    vga_text_writeline(&terminal,
        "kernel_intro.asm writes 0x92 for kernel data, but it reads back 0x93.");
    vga_text_writeline(&terminal,
        "Bit 0 is ACCESSED, and the processor sets it when the descriptor is");
    vga_text_writeline(&terminal,
        "loaded into a segment register. The TSS shows 0x8B, not 0x89, because");
    vga_text_writeline(&terminal,
        "ltr also sets bit 1: BUSY. The hardware writes to your GDT.");
    vga_text_set_color(&terminal, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_text_writeline(&terminal, "");

    /* 3. Which selectors is the kernel actually running with? */
    uint16_t cs, ds, ss;
    asm volatile("mov %%cs, %0" : "=r"(cs));
    asm volatile("mov %%ds, %0" : "=r"(ds));
    asm volatile("mov %%ss, %0" : "=r"(ss));

    hex("running with CS = ", cs);
    hex("             DS = ", ds);
    hex("             SS = ", ss);
    vga_text_writeline(&terminal,
        "low 2 bits of CS are the RPL: 0 here, so we are in ring 0");

    for (;;);
}
