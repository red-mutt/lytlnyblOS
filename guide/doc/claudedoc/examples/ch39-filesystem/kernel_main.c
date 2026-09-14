/* ============================================================
 * Chapter 39 -- The filesystem, from the kernel side
 *
 * Directories, files, writing, reading back, and the one sharp
 * edge: there is no lseek, so an open file only moves forwards.
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

static void step(const char* msg) {
    vga_text_set_color(&terminal, VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
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
    fs_manager_init();      /* this also brings up the ATA driver */

    vga_text_set_color(&terminal, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_text_clear(&terminal);

    /* 1. Make a directory and a file inside it. Paths must be
     *    absolute: there is no working directory and no "." or "..". */
    step("mkdir /notes, touch /notes/todo.txt");
    fs_mkdir("/notes");
    fs_touch("/notes/todo.txt");

    /* 2. Open it. The returned descriptor is >= 3, because 0, 1 and 2
     *    are reserved for keyboard and screen. -1 means failure. */
    int32_t fd = fs_open("/notes/todo.txt");
    vga_text_write(&terminal, "fs_open returned fd ");
    vga_text_write_dec(&terminal, fd);
    vga_text_writeline(&terminal, "");

    if (fd < 0) {
        vga_text_writeline(&terminal, "could not open the file");
        for (;;);
    }

    /* 3. Write to it. The descriptor carries an offset that advances
     *    with every write. */
    const char* text = "buy milk";
    int32_t written = fs_write(fd, (void*)text, 8);
    vga_text_write(&terminal, "wrote ");
    vga_text_write_dec(&terminal, written);
    vga_text_writeline(&terminal, " bytes");

    /* 4. To read what we just wrote we must CLOSE AND REOPEN.
     *    The offset is now at 8, there is no lseek to rewind it,
     *    and reading at or past the end returns 0. */
    fs_close(fd);
    fd = fs_open("/notes/todo.txt");

    step("reading it back");
    char buffer[32];
    memset(buffer, 0, sizeof(buffer));
    int32_t got = fs_read(fd, buffer, sizeof(buffer) - 1);

    vga_text_write(&terminal, "read ");
    vga_text_write_dec(&terminal, got);
    vga_text_write(&terminal, " bytes: \"");
    vga_text_write(&terminal, buffer);
    vga_text_writeline(&terminal, "\"");

    /* 5. Reading again from the same descriptor returns 0: end of
     *    file, which is not an error. That distinction is what makes
     *    the shell's cat loop terminate correctly. */
    int32_t again = fs_read(fd, buffer, sizeof(buffer) - 1);
    vga_text_write(&terminal, "reading again returns ");
    /* Printed as a string rather than with vga_text_write_dec: that
     * function has a bug for the value 0, where it writes the '0' but
     * forgets to advance the cursor, so the next text covers it up. */
    if (again == 0) {
        vga_text_writeline(&terminal, "0  (EOF, which is not an error)");
    } else {
        vga_text_write_dec(&terminal, again);
        vga_text_writeline(&terminal, "  (unexpected)");
    }
    fs_close(fd);
    vga_text_writeline(&terminal, "");

    /* 6. Listing. fs_ls prints straight to the terminal. */
    step("contents of /");
    fs_ls("/");
    step("contents of /notes");
    fs_ls("/notes");
    vga_text_writeline(&terminal, "");

    /* 7. Deleting. */
    step("rm /notes/todo.txt, then list again");
    fs_rm("/notes/todo.txt");
    fs_ls("/notes");

    for (;;);
}
