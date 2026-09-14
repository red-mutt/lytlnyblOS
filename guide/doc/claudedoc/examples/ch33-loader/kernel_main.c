/* ============================================================
 * Chapter 33 -- the program loader
 *
 * load_program() does everything needed to turn a file on disk
 * into a running ring 3 process: read it, create an address space,
 * copy the code into fresh frames, map them at 0x400000, and put
 * the process on the scheduler's list.
 *
 * Here we load two programs from the kernel, with no shell
 * involved, and watch them run.
 * ============================================================ */

#include "kernel/drivers/vga_text.h"
#include "kernel/interrupts.h"
#include "kernel/drivers/timer.h"
#include "kernel/drivers/keyboard.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "memory/heap.h"
#include "tasks/procman.h"
#include "tasks/loader.h"
#include "filesystem/fs_manager.h"
#include "kernel/mappings.h"

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
    vga_text_writeline(&terminal, "-- loading programs from the kernel --");
    vga_text_writeline(&terminal, "");

    /* 1. A path that does not exist. load_program returns NULL, and
     *    the caller MUST check: the version of this check missing in
     *    the FS_RUN system call is why `run /nosuchfile` hangs the
     *    shell forever. */
    process_t* missing = load_program("/bin/does_not_exist");
    if (missing == NULL) {
        vga_text_writeline(&terminal,
            "load_program(\"/bin/does_not_exist\") returned NULL, as it should");
    } else {
        vga_text_writeline(&terminal, "unexpected: got a process back");
    }
    vga_text_writeline(&terminal, "");

    /* 2. A real program. */
    process_t* p = load_program("/bin/user_test");
    if (p == NULL) {
        vga_text_writeline(&terminal, "could not load /bin/user_test");
        for (;;);
    }

    vga_text_write(&terminal, "loaded /bin/user_test as pid ");
    vga_text_write_dec(&terminal, p->pid);
    vga_text_writeline(&terminal, "");

    show("  its page directory: ", (uint32_t)p->page_directory);
    show("  entry point (EIP):  ", p->regs.eip);
    show("  stack pointer:      ", p->regs.esp);
    show("  code segment (CS):  ", p->regs.cs);
    vga_text_writeline(&terminal,
        "  CS ends in 3, so it will run in ring 3");
    vga_text_writeline(&terminal, "");

    /* The process is READY but has never run. Nothing else is
     * needed: the timer interrupt will reach the scheduler, the
     * scheduler will pick it, and load_context + iret will drop the
     * CPU into ring 3 at 0x400000. */
    vga_text_writeline(&terminal, "it is now READY. waiting for the scheduler:");
    vga_text_writeline(&terminal, "");

    for (;;);
}
