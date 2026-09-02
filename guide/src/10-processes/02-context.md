# Context Switching

## Context

Not much context is needed for this section, all we need to know
is that what we are making is simply used to switch between the process
data structures that we have made.

Actually there's one major issue that comes with making a context
switcher, and we experienced it partially when creating the main kernel
process. This is the issue of how we can get the `ESP` and `EIP` values when
the code currently being executed is switching the process. We
wouldn't be able to get ESP and EIP for our actual process that we want
to be saving.

The solution for this has been lying right under our noses. It's
interrupts, these save a state of the CPU when called and then return to
the previous state by popping out the `registers_t` structure (the one
that we defined in the interrupts file, not the process manager file).
We won't have a specific interrupt for the context switcher, we will
just use the timer for now, as this would be where we handle scheduling
too.

## Coding

Our header is small:

```c
#ifndef CONTEXT_H
#define CONTEXT_H

#include "../tasks/procman.h"
#include <stdint.h>


void context_switch(kprocess_t* old_process, 
        kprocess_t* new_process, 
        registers_t* regs);

void save_context(kprocess_t* process, registers_t* regs);

void load_context(kprocess_t* process, registers_t* regs);

#endif
```

And the C file here also doesn't really need much explanation either

```c
#include "context.h"

void save_context(kprocess_t* process, registers_t* regs) {
    process->regs.eip = regs->eip;
    process->regs.cs = regs->cs;
    process->regs.eflags = regs->eflags;
    process->regs.ds = regs->ds;

    process->regs.edi = regs->edi;
    process->regs.esi = regs->esi;
    process->regs.ebp = regs->ebp;
    process->regs.esp = regs->esp;
    process->regs.ebx = regs->ebx;
    process->regs.edx = regs->edx;
    process->regs.ecx = regs->ecx;
    process->regs.eax = regs->eax;
}

void load_context(kprocess_t* process, registers_t* regs) {
    regs->eip = process->regs.eip;
    regs->cs = process->regs.cs;
    regs->eflags = process->regs.eflags;
    regs->ds = process->regs.ds;

    regs->edi = process->regs.edi;
    regs->esi = process->regs.esi;
    regs->ebp = process->regs.ebp;
    regs->esp = process->regs.esp;
    regs->ebx = process->regs.ebx;
    regs->edx = process->regs.edx;
    regs->ecx = process->regs.ecx;
    regs->eax = process->regs.eax;
}

void context_switch(kprocess_t* old_process, kprocess_t* new_process, registers_t* regs) {
    current_process = new_process;
    save_context(old_process, regs);
    load_context(new_process, regs);
}
```

Notice that `context_switch()` itself doesn't directly change the CPU's registers.
Instead, it changes the values inside the `registers_t` structure that
the interrupt handler will later restore. This works because the context switch is 
happening from inside a timer interrupt, so the interrupt return mechanism gives
us a way to load the new process's saved CPU state.

We then also need to make an infrastructure for calling these functions
using our timer, I'll just paste the full edited file, seeing as it's
still small anyway:

```c
#include "timer.h"
#include "interrupts.h"
#include "vga_text.h"

volatile uint32_t ticks = 0;
static uint32_t freq;

//context switcher stuff
volatile bool context_switch_requested = false;
volatile uint32_t old_process_pid;
volatile uint32_t new_process_pid;

extern vga_text terminal;

void timer_init(uint32_t frequency) {
    freq = frequency;
    uint16_t divisor = 1193182 / frequency;

    /* tell pit how we send the divisor value and the mode*/
    outb(PIT_COMMAND, PIT_ACCESS_LOHIBYTE | PIT_MODE3 | PIT_CHANNEL0 | PIT_BINARY);
    io_wait();

    /* write low and high bytes respectively */
    outb(PIT_CHANNEL0_DATA, divisor & 0xFF);
    io_wait();
    outb(PIT_CHANNEL0_DATA, divisor >> 8);
    io_wait();
}

void timer_handler(registers_t* regs) {
    ticks++;
    if ((ticks % 100) == 0) {
        //vga_text_writeline(&terminal, " 1 second ");
    }

    if (context_switch_requested) {
        context_switch(find_process_by_pid(old_process_pid), find_process_by_pid(new_process_pid), regs);
        context_switch_requested = false;
    }
}

uint64_t timer_get_ticks() {
    return ticks;
}

void timer_wait_ms(uint32_t ms) {
    uint32_t start = ticks;

    while ((ticks - start) < ms) {
        asm volatile ("hlt");
    }
}
```

Then if we make those 3 global variables public by putting them in our
header like this

```c
extern volatile bool context_switch_requested;
extern volatile uint32_t old_process_pid;
extern volatile uint32_t new_process_pid;
```

We can then request a context switch from anywhere in our code.

If you're confused how loading the process works by simply just loading
process data into the `registers_t` structure, this is because when the
IRQ wants to return after the timer interrupt is done, it pops all the
data from the `registers_t` structure and then uses this to return to the
previous place in code execution, so we can also use interrupts to our
advantage to load processes, as well as save them.

## Simple test

I'll just show you the whole of `main` to show you how simple
of a test this is.
```c
#include "vga_text.h"
#include "interrupts.h"
#include "timer.h"
#include "keyboard.h"
#include "../memory/pmm.h"
#include "../memory/vmm.h"
#include "../memory/heap.h"
#include "../tasks/procman.h"

#include  "<stdint.h>

vga_text terminal;

void test_process() {
    vga_text_writeline(&terminal, "PROCESS RUNNING");
    old_process_pid = 2;
    new_process_pid = 1;
    context_switch_requested = true;
    for (;;);
}

void kernel_main(void)
{
    volatile char* vga = (volatile char*)0xB8000;
    
    //signal that we have reached C
    vga[0] = 'C';
    vga[1] = 0x02;

    vga_text_init(&terminal);
    vga_text_writeline(&terminal, "Welcome to the lytlnybl kernel in protected mode");

    idt_init();
    timer_init(100);
    keyboard_init();
    init_pmm(); 
    init_vmm();
    init_heap();
    init_procman();

    uint32_t* numbers = (uint32_t*)kmalloc(5 * sizeof(uint32_t));

    for (int i = 0; i < 5; i++) {
        numbers[i] = (i + 1) * 10; // Stores 10, 20, 30, 40, 50
    }

    vga_text_write(&terminal, "Values: ");
    for (int i = 0; i < 5; i++) {
        vga_text_write_dec(&terminal, numbers[i]);
        vga_text_write(&terminal, " ");
    }
    vga_text_writeline(&terminal, "");

    kfree(numbers);

    kprocess_t* test_proc = create_kprocess(test_process);
    old_process_pid = 1;
    new_process_pid = test_proc->pid;
    context_switch_requested = true;
    timer_wait_ms(10);

    vga_text_writeline(&terminal, "back in main");
    

    for (;;);
}
```

As you can see, we just define a process for our `test_process` function,
we switch, and then switch back. This should work. You may notice that
there is also a new function, from the timer, this being `timer_wait_ms`
(remember to define this in the timer's header too). The reason this exists and is
used is that the code for printing that we are back in main will happen before
the context switch happens. This is because the context switch only happens
when the timer interrupt fires. We therefore wait for a little while
to give the timer a chance to perform the context switch. If everything is good
you should be seeing the text showing appropriately.

