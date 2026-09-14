/* ============================================================
 * Chapter 24 -- reading from the keyboard
 *
 * read(0, ...) blocks: the process is parked, the scheduler runs
 * something else, and the keyboard interrupt fills the buffer and
 * wakes it. None of that is visible from here -- which is the point.
 *
 * Run it with:  run /bin/user_test
 * ============================================================ */

#include <stdint.h>
#include "user/libc/output.h"
#include "user/libc/syscalls.h"
#include "user/libc/memory.h"
#include "user/libc/string.h"

void _start(void)
{
    char line[64];

    printf("type a line and press Enter:\n");

    /* The kernel fills the buffer and returns how many bytes it
     * wrote. It does NOT add a terminating zero: that is the
     * caller's job, and forgetting it is the bug the shell has.
     *
     * Leave room for the terminator by reading one less than the
     * buffer holds. */
    int n = read(0, line, sizeof(line) - 1);

    if (n < 0) {
        printf("read failed\n");
        exit();
    }

    /* The line includes the Enter that ended it. Drop it. */
    if (n > 0 && line[n - 1] == '\n') {
        n--;
    }
    line[n] = '\0';          /* now, and only now, it is a C string */

    printf("read %d bytes\n", n);
    printf("you typed: [%s]\n", line);
    printf("strlen agrees: %d\n", strlen(line));

    /* Proof that the terminator mattered: strlen only works because
     * we wrote it. Without that line, strlen would run off the end
     * of the buffer looking for a zero that might be anywhere. */

    exit();
}
