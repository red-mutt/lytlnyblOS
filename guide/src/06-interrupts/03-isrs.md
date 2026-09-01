# Adding all the ISRs

Before we make the other ISRs, we need to develop our current solution a little
further. We can create a common stub that saves the processor state and passes
it to our C interrupt handler. Since every ISR needs to perform this same setup,
using a common stub prevents us from having to duplicate the same code
for every interrupt. This stub pushes all our needed registers onto 
the stack to be used as the parameter

```x86asm
[BITS 32]

extern isr_handler

global idt_load
global isr0

idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret

isr_common_stub:
    pusha

    mov ax, ds
    push eax

    push esp
    call isr_handler
    add esp, 4

    pop eax

    popa

    add esp, 8
    iret

isr0:
    cli

    ; error code
    push dword 0

    ; interrupt number
    push dword 0

    jmp isr_common_stub
```

If you're wondering why we don't push registers such as `EIP`, `CS` or 
`EFLAGS`, that is because the CPU automatically pushes them onto the stack
when the interrupt occurs. Our stub only needs to save the general-purpose registers
and the data-segment register that we want to make available to the C handler. 
Also, another thing to consider is that passing memory from assembly
to C can be confusing, this is why I covered the stack prior. We may
find some bugs relating to that in future. It may be good to print your
values received in C and ensure that they are correct.

We are creating ISR1-ISR31. This is tedious, which is exactly why we can use
assembly macros to generate most of the repetitive code. There's one important
distinction: some exceptions automatically push an error code onto the stack
while others do not. Our two macros account for this difference.

Add this to the header file:

```c
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);
```

And this to the implementation file:

```c
idt_set_gate(0,  (uint32_t)isr0,  0x08, 0x8E);
idt_set_gate(1,  (uint32_t)isr1,  0x08, 0x8E);
idt_set_gate(2,  (uint32_t)isr2,  0x08, 0x8E);
idt_set_gate(3,  (uint32_t)isr3,  0x08, 0x8E);
idt_set_gate(4,  (uint32_t)isr4,  0x08, 0x8E);
idt_set_gate(5,  (uint32_t)isr5,  0x08, 0x8E);
idt_set_gate(6,  (uint32_t)isr6,  0x08, 0x8E);
idt_set_gate(7,  (uint32_t)isr7,  0x08, 0x8E);
idt_set_gate(8,  (uint32_t)isr8,  0x08, 0x8E);
idt_set_gate(9,  (uint32_t)isr9,  0x08, 0x8E);
idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);
```

And then add this to assembly:

```x86asm

global isr0
global isr1
global isr2
global isr3
global isr4
global isr5
global isr6
global isr7
global isr8
global isr9
global isr10
global isr11
global isr12
global isr13
global isr14
global isr15
global isr16
global isr17
global isr18
global isr19
global isr20
global isr21
global isr22
global isr23
global isr24
global isr25
global isr26
global isr27
global isr28
global isr29
global isr30
global isr31

%macro ISR_NOERRCODE 1
isr%1:
    push dword 0
    push dword %1
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
isr%1:
    push dword %1
    jmp isr_common_stub
%endmacro

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7

ISR_ERRCODE 8

ISR_NOERRCODE 9

ISR_ERRCODE 10
ISR_ERRCODE 11
ISR_ERRCODE 12
ISR_ERRCODE 13
ISR_ERRCODE 14

ISR_NOERRCODE 15
ISR_NOERRCODE 16

ISR_ERRCODE 17

ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29

ISR_ERRCODE 30

ISR_NOERRCODE 31
```

This code block uses NASM macros, which act as a simple code generator
during assembly. Each time we invoke one of these macros, NASM expands it into
the instructions defined inside the macro. We have two macros because
the CPU handles error codes differently for different exceptions. `ISR_NOERRCODE` pushes
a dummy error code so that the stack has the same layout as an exception that provides one,
while `ISR_ERRCODE` relies on the CPU's existing error code and 
only pushes the interrupt number.

We should be able to run our code again and isr0 should still work,
another thing we need to do is add a printing for each exception, here
is a nice array you can use:

```c
const char* exception_messages[32] =
{
    "Divide By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    "Reserved"
};
```

The interrupt number stored in `regs->interrupt_number` tells us which
exception occurred. Since our array is indexed from zero, we can use that
number directly to select the appropriate message. This gives us a nice and 
simple way to turn an exception number into something readable on the screen.

And then in the ISR handler we can just have

```c
vga_text_writeline(&terminal, exception_messages[regs->interrupt_number]);
```

And now we can run some tests:

We should then test our exceptions to make sure the IDT
and ISR stubs are working correctly. You do not need to trigger all 32,
but testing a reasonable selection is useful. You can also look up the exceptions
deliberately trigger some of them with assembly instructions. This is a good
way to become familiar with how the CPU transfers control to the handlers.
Exception 3, the breakpoint exception, is particularly useful because it can also
be triggered with the `int 3` instruction and used while debugging.


