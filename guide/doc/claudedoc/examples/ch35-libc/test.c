/* ============================================================
 * Chapter 35 -- writing a user program
 *
 * A template. Everything a program in this system can do, in one
 * file: output, the heap, strings, and the process system calls.
 *
 * Build: the Makefile compiles user/programs/test.c, links it
 * against libc.a with user.ld, and mkfs copies the result into the
 * image as /bin/user_test.
 * ============================================================ */

#include <stdint.h>
#include "user/libc/output.h"
#include "user/libc/syscalls.h"
#include "user/libc/memory.h"
#include "user/libc/string.h"

/* The entry point MUST be called _start: it is what user.ld names,
 * and the kernel jumps to whatever sits at 0x400000. The shell gets
 * away with calling it start() only because its object file happens
 * to be linked first. Do not copy that habit. */
void _start(void)
{
    /* ---- output ---- */
    printf("%s\n", "printf understands %s, %d, %c and %%");
    printf("a number: %d, a char: %c, a literal percent: %%\n", -42, 'X');
    puts("puts writes a string with no formatting\n");

    /* ---- who am I ---- */
    printf("my pid is %d\n", get_pid());

    /* ---- the heap ---- */
    char* a = malloc(32);
    if (a == NULL) { printf("out of memory\n"); exit(); }
    strcpy(a, "malloc then strcpy");
    printf("%s\n", a);

    /* calloc zeroes what it hands you; malloc does not. */
    int* counts = calloc(4, sizeof(int));
    counts[3] = 7;
    printf("calloc gave zeros, then we set [3]=%d\n", counts[3]);

    /* realloc keeps the contents and may move the block. Always
     * assign the result: on failure it returns NULL and the old
     * pointer is still valid, so overwriting it would leak. */
    char* grown = realloc(a, 64);
    if (grown != NULL) {
        a = grown;
        strcat(a, " then realloc");
        printf("%s\n", a);
    }

    free(a);
    free(counts);

    /* ---- strings ---- */
    printf("strlen(\"lytlnybl\") = %d\n", strlen("lytlnybl"));
    printf("strcmp(\"a\",\"a\") = %d\n", strcmp("a", "a"));

    /* ---- the process calls ---- */
    printf("sleeping 50 ticks...\n");
    sleep(50);              /* blocks; the CPU runs something else */
    printf("awake\n");

    yield();                /* give up the rest of this time slice */

    /* exit() never returns. Without it the function would run off
     * the end and try to return to an address that does not exist. */
    printf("calling exit()\n");
    exit();
}
