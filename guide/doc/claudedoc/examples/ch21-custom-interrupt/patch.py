#!/usr/bin/env python3
"""
Chapter 21 -- adding an interrupt vector of your own.

The kernel registers vectors 0-31 (CPU exceptions), 32-47 (hardware
IRQs) and 0x80 (system calls). Everything else in the 256-entry IDT is
zero, which means "not present": triggering one raises a general
protection fault instead.

Here we add vector 0x30 (48). Three edits, one per layer:

    1. an assembly stub, because C cannot end a function with iret
    2. a declaration so C can take the stub's address
    3. an IDT entry pointing at it, and a case to handle it

Then `int 0x30` from anywhere in the kernel lands in our handler.
"""

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                "..", "common"))
from patchlib import run


def apply(p):
    # ------------------------------------------------------------
    # 1. The stub. It mimics ISR_NOERRCODE: push a fake error code,
    #    push the vector number, then join the common path that saves
    #    every register and calls isr_handler.
    # ------------------------------------------------------------
    p.after("kernel/interrupts.asm",
            "ISR_NOERRCODE 31",
            """
global isr48
isr48:
    push dword 0        ; fake error code, so the stack layout matches
    push dword 48       ; the vector number
    jmp isr_common_stub""")

    # ------------------------------------------------------------
    # 2. Let C see it.
    # ------------------------------------------------------------
    p.after("include/kernel/interrupts.h",
            "extern void syscall_entry(void);",
            "extern void isr48(void);")

    # ------------------------------------------------------------
    # 3a. Register it. 0x8E = present, DPL 0, 32-bit interrupt gate:
    #     ring 3 may NOT trigger this one. Use 0xEE if you want user
    #     programs to be able to call it, as the syscall gate does.
    # ------------------------------------------------------------
    p.after("kernel/interrupts.c",
            "    idt_set_gate(0x80, (uint32_t)syscall_entry, 0x08, 0xEE);",
            "    idt_set_gate(0x30, (uint32_t)isr48, 0x08, 0x8E);")

    # ------------------------------------------------------------
    # 3b. Handle it. This must come before the default branch, which
    #     would index exception_messages[48] -- an array of 32.
    #
    #     Note the `return`: the code after the switch halts the
    #     kernel forever, because a fault in ring 0 has no safe
    #     recovery. Ours is not a fault, so we return and let the
    #     stub's iret resume the interrupted code.
    # ------------------------------------------------------------
    p.before("kernel/interrupts.c",
             "        default:\n            vga_text_writeline(&terminal, exception_messages[regs->interrupt_number]);",
             """        case 48:
            vga_text_set_color(&terminal, VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
            vga_text_writeline(&terminal, "  --> our handler for vector 0x30 ran");
            vga_text_set_color(&terminal, VGA_COLOR_WHITE, VGA_COLOR_BLACK);

            /* regs points at the interrupted code's saved state, on
             * the stack. Reading it shows who called us; writing to
             * it changes what they see when they resume. */
            vga_text_write(&terminal, "      it was called from EIP ");
            vga_text_write_hex(&terminal, regs->eip);
            vga_text_write(&terminal, ", with EBX = ");
            vga_text_write_hex(&terminal, regs->ebx);
            vga_text_writeline(&terminal, "");

            regs->ebx = 0x5A5A5A5A;   /* the caller will see this */
            return;                   /* NOT break: see the comment above */
""")


sys.exit(run(apply))
