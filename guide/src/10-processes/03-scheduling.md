# Scheduler

## Context

The scheduler currently is going to be simple to make, this is because we
already have our context switcher and process manager. The scheduler just
simply decides what process ought to be run next. When making the scheduler, we
can abstract away considering things like, page allocation, the process's
stack, the heap and all sorts of stuff, the only things that we need to
consider are the current process, the linked list of all processes and each
process's state.

The scheduling algorithm we are going to be using is round-robin, if you are
unfamiliar with this it gives each process a time slice (basically an amount
of ticks) to execute, and then you cycle through executing all the available
processes for the given time slice.

## The code

```c
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "procman.h"
#include "../kernel/interrupts.h"
#include "context.h"

#define DEFAULT_TIME_SLICE 10

kprocess_t* get_next_process();

void schedule(registers_t* regs);

#endif
```

Just like the context switcher, this is a small header file, the single
definition we make is the default amount of timer ticks that it takes to
switch the process. The implementation for this is just as small:

```c
#include "scheduler.h"

volatile uint32_t scheduler_tick_count = 0;
uint32_t time_slice = DEFAULT_TIME_SLICE;

kprocess_t* get_next_process() {
    kprocess_t* traversal_process = current_process;
    do {
        if (traversal_process->state == PROCESS_READY) {
            return traversal_process;
        }

        if (!traversal_process->next) {
            traversal_process = process_head;
        } else {
            traversal_process = traversal_process->next;
        }
    } while (!(traversal_process->state == PROCESS_RUNNING));

    return current_process;
}

void schedule(registers_t* regs) {
    scheduler_tick_count++;
    if (scheduler_tick_count >= time_slice) { 
        scheduler_tick_count = 0;
        kprocess_t* next_process = get_next_process();
        if (next_process != current_process) {
            context_switch(current_process, next_process, regs);
        }
    }
    return;
}
```

### `get_next_process`

This is just a traversal algorithm that just cycles through the list until it
finds the next ready process. If it traverses to the running process again then
it just returns that instead.

### `schedule`

This is the main thing we are calling from the timer to schedule. If
our scheduler tick count goes up to the value we have in our `time_slice` then we
execute the bulk of the function. This is just where we find the next process
using the `get_next_process` function, and then if it's not the same as our
current process we perform a context switch.

## Refactoring timer and main

Let's have a look at the new timer:

```c
#include "timer.h"
#include "interrupts.h"
#include "vga_text.h"

volatile uint32_t ticks = 0;
static uint32_t freq;

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

    schedule(regs);
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

We actually just remove a lot of the previous stuff and just replace it with the
schedule function. Nice! Negative code added! And then next we have the
changes to main, this is also just removing stuff:

```c
#include "vga_text.h"
#include "interrupts.h"
#include "timer.h"
#include "keyboard.h"
#include "../memory/pmm.h"
#include "../memory/vmm.h"
#include "../memory/heap.h"
#include "../tasks/procman.h"

#include 

vga_text terminal;

void test_process() {
    vga_text_writeline(&terminal, "PROCESS RUNNING");
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

    init_pmm();
    init_vmm();
    init_heap();
    init_procman();

    timer_init(100);
    keyboard_init();

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

    kprocess_t* test_proc= create_kprocess(test_process);
    timer_wait_ms(10);

    vga_text_writeline(&terminal, "back in main");
    

    for (;;);
}
```

And this should work, next up is user management! We will next actually have an 
operating system and not just a kernel.
