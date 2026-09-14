#!/usr/bin/env python3
"""
Chapter 34 -- adding a system call, end to end.

Adding a service to the kernel touches four files, and this script
makes each of the four edits. Read it as the recipe: these are exactly
the four things you have to do, in order.

We add SYSCALL_UPTIME (0x0B), which returns the number of timer ticks
since boot -- one line of actual work, wrapped in the full ceremony of
crossing the user/kernel boundary.
"""

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                "..", "common"))
from patchlib import run


def apply(p):
    # ------------------------------------------------------------
    # 1. Give the call a number, on the KERNEL side.
    #    The numbers are just a private agreement between the two
    #    sides; 0x0A was the last one taken.
    # ------------------------------------------------------------
    p.after("include/kernel/interrupts.h",
            "#define SYSCALL_CLOSE 0x0A",
            "#define SYSCALL_UPTIME 0x0B")

    # ------------------------------------------------------------
    # 2. Handle it in the dispatcher.
    #    regs->eax is both the number coming in and the result going
    #    out: writing to it changes what the user program sees in EAX
    #    after iret. interrupts.c already includes timer.h.
    # ------------------------------------------------------------
    p.before("kernel/interrupts.c",
             "        case SYSCALL_GETPID:",
             "        case SYSCALL_UPTIME:\n"
             "            regs->eax = (uint32_t)timer_get_ticks();\n"
             "            break;")

    # ------------------------------------------------------------
    # 3. Give the call the same number on the USER side, and declare
    #    the wrapper so programs can see it.
    # ------------------------------------------------------------
    p.after("include/user/libc/syscalls.h",
            "#define SYSCALL_CLOSE 0x0A",
            "#define SYSCALL_UPTIME 0x0B")

    p.after("include/user/libc/syscalls.h",
            "void close(int fd);",
            "uint32_t uptime(void);")

    # ------------------------------------------------------------
    # 4. Write the wrapper itself. syscall() puts the number in EAX,
    #    the three arguments in EBX/ECX/EDX, runs int 0x80, and
    #    returns whatever EAX holds afterwards.
    # ------------------------------------------------------------
    p.append("user/libc/syscalls.c",
             "\nuint32_t uptime(void) {\n"
             "  return syscall(SYSCALL_UPTIME, 0, 0, 0);\n"
             "}")


sys.exit(run(apply))
