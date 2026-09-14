/* ============================================================
 * Chapter 34 -- a user program calling the new system call.
 *
 * This replaces user/programs/test.c, so it becomes /bin/user_test.
 * Run it from the shell with:   run /bin/user_test
 * ============================================================ */

#include <stdint.h>
#include "user/libc/output.h"
#include "user/libc/syscalls.h"

void _start(void)
{
    printf("uptime syscall demo\n");

    uint32_t first = uptime();
    printf("ticks now: %d\n", first);

    /* sleep() is itself a system call: it parks this process and the
     * timer wakes it, so the CPU is free meanwhile. 100 ticks at
     * 100 Hz is one second. */
    sleep(100);

    uint32_t second = uptime();
    printf("ticks after sleeping 100: %d\n", second);
    printf("elapsed: %d ticks\n", second - first);

    exit();
}
