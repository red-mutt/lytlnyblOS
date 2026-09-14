/* ============================================================
 * Chapter 37 -- the disk, underneath the filesystem
 *
 * ata_read_sector and ata_write_sector move exactly 512 bytes at
 * an LBA you choose. No files, no names, no allocation: just a
 * numbered array of blocks.
 *
 * We use LBA 2000, which is far past the filesystem (that starts
 * at sector 70 and holds 1000 blocks) and far past the kernel
 * (sectors 1-60). Writing inside either would corrupt the system.
 * ============================================================ */

#include "kernel/drivers/vga_text.h"
#include "kernel/interrupts.h"
#include "kernel/drivers/timer.h"
#include "kernel/drivers/keyboard.h"
#include "kernel/drivers/ata.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "memory/heap.h"
#include "tasks/procman.h"
#include "filesystem/fs_manager.h"
#include "kernel/kernel_utils.h"

#include <stdint.h>

vga_text terminal;

#define SCRATCH_LBA 2000

static void ok(const char* m) {
    vga_text_set_color(&terminal, VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_text_writeline(&terminal, m);
    vga_text_set_color(&terminal, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}
static void bad(const char* m) {
    vga_text_set_color(&terminal, VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_text_writeline(&terminal, m);
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
    fs_manager_init();      /* calls init_ata() for us */

    vga_text_set_color(&terminal, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_text_clear(&terminal);
    vga_text_writeline(&terminal, "-- raw disk sectors --");
    vga_text_writeline(&terminal, "");

    /* Buffers must be 512 bytes: the driver always moves exactly one
     * sector, 256 16-bit words through port 0x1F0. */
    uint8_t out[512];
    uint8_t in[512];

    /* 1. Fill a recognisable pattern and write it. */
    for (int i = 0; i < 512; i++)
        out[i] = (uint8_t)(i & 0xFF);

    if (!ata_write_sector(SCRATCH_LBA, out)) {
        bad("ata_write_sector failed");
        for (;;);
    }
    vga_text_writeline(&terminal, "wrote 512 bytes to LBA 2000");

    /* 2. Read it back into a different buffer. */
    memset(in, 0, sizeof(in));
    if (!ata_read_sector(SCRATCH_LBA, in)) {
        bad("ata_read_sector failed");
        for (;;);
    }
    vga_text_writeline(&terminal, "read it back");

    /* 3. Compare. */
    int mismatch = -1;
    for (int i = 0; i < 512; i++) {
        if (in[i] != out[i]) { mismatch = i; break; }
    }

    if (mismatch < 0) {
        ok("all 512 bytes match: the data really reached the platter");
    } else {
        vga_text_write(&terminal, "mismatch at byte ");
        vga_text_write_dec(&terminal, mismatch);
        vga_text_writeline(&terminal, "");
    }
    vga_text_writeline(&terminal, "");

    /* 4. Look at a sector that belongs to the filesystem. Block 0 of
     *    the filesystem is at disk sector 70, and its first four
     *    bytes are the magic number that tells the driver this disk
     *    has been formatted. */
    if (ata_read_sector(70, in)) {
        uint32_t magic = ((uint32_t)in[0])
                       | ((uint32_t)in[1] << 8)
                       | ((uint32_t)in[2] << 16)
                       | ((uint32_t)in[3] << 24);
        vga_text_write(&terminal, "sector 70, first 4 bytes: ");
        vga_text_write_hex(&terminal, magic);
        vga_text_writeline(&terminal, "");

        if (magic == 0xDEADBABE) {
            ok("that is FS_MAGIC: the superblock, seen as raw bytes");
        } else {
            bad("not the magic number");
        }
    }

    vga_text_writeline(&terminal, "");
    vga_text_set_color(&terminal, VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
    vga_text_writeline(&terminal,
        "everything the filesystem does is built on these two functions");
    vga_text_set_color(&terminal, VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    for (;;);
}
