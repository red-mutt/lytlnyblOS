# System Calls

Now it's time to make a user space somewhat useful. A system 
call is a way for a user process to request certain functionality
from the kernel. Our set of system calls (often called syscalls)
will not meet the POSIX standards, and will be an incredibly simple
implementation. The good part of this is that it's simple to extend
the set of syscalls that we have. We can simply add more as our operating system
requires more functionality.
Our set of syscalls will be:
`exit, getpid, yield, sleep, write, read, sbrk`

## The context

### How do we get into the kernel?

Simple; the answer is interrupts! In this section we will be making our
first software interrupt. For our syscalls we will move the values
that the syscalls require into `EAX, EBX, ECX, EDX`. Then we can
access these values through our interrupt register structure. We can use the same
interrupt for each syscall and then `EAX` will just contain a value that 
identifies the syscall.

Let's look into each one of the syscalls we'll make and how
they're supposed to work

### `void exit(void)`

The simplest syscall out of them all. We just set the process to terminated and
perform a context switch so that it's removed. If we don't context switch,
the terminated process may continue executing until the scheduler runs again,
so we don't want to do that.

### `uint32_t get_pid(void)`

It's obvious what this does: it gets the PID for the currently running process that 
called it. We can return the PID by putting the value into `EAX`.

### `void yield(void);`

A manual way to perform a context switch from a user process. It doesn't
specify which process to switch to; it just switches to the next one in the list.

### `void sleep(uint32_t ticks)`

Makes the process sleep for a certain amount of ticks. For this, you will have
to store some new data in the process data type, because the syscall will mark it as
sleeping, set how long it should sleep for, and then perform a context switch.

### `int write(int fd, void *buff, size_t count)`

For now, this function will only be used to write to the terminal using
`fd = 1`, this is because we don't currently have a file system and a way
to set our file descriptors. I currently don't see a point to implement
a `stderr` right about now.

### `int read(int fd, void* buff, size_t count)`

Like the previous one, this will just read input from the keyboard via `stdin`.
Like the sleep syscall, we will have to block the process until the requested number of 
characters has been entered. This will also require more data to be stored for
each process structure to capture the reading state.

### `void* sbrk (intptr_t increment)`

This function is used to grow the heap, which we currently don't have. We can
just give the heap a fixed starting address and initially allocate one page for it.
This will require more data to be stored in each process
about how many pages are allocated to the heap and about the address
where the heap ends. For our implementation, this function will return
the new address at the end of the heap. 
We don't have to worry about allocation or handling of the
heap in this section. This will be handled in the next one.

## The implementation

Here's the header for syscalls.h:

```c
#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>
#include <stddef.h>

#define SYSCALL_EXIT 0x01
#define SYSCALL_GETPID 0x02
#define SYSCALL_YIELD 0x03
#define SYSCALL_SLEEP 0x04
#define SYSCALL_WRITE 0x05
#define SYSCALL_READ 0x06
#define SYSCALL_SBRK 0x07

extern uint32_t syscall(uint32_t CODE, uint32_t a, uint32_t b, uint32_t c);

void exit(void);
uint32_t get_pid(void);
void yield(void);
void sleep(uint32_t ticks);
int write(int fd, void *buff, size_t count);
int read(int fd, void* buff, size_t count);
void* sbrk (intptr_t increment);


#endif

And here is the implementation file:

#include "syscalls.h"

void exit() {
  syscall(SYSCALL_EXIT, 0, 0, 0);
} //get warning here due to function name maybe, ignore it

uint32_t get_pid() {
  return syscall(SYSCALL_GETPID, 0, 0, 0);
}

void yield() {
  syscall(SYSCALL_YIELD, 0, 0, 0);
}

void sleep(uint32_t ticks) {
  syscall(SYSCALL_SLEEP, ticks, 0, 0);
}

int write(int fd, void *buff, size_t count) {
  return syscall(SYSCALL_WRITE, fd, (uint32_t)buff, count);
}


int read(int fd, void* buff, size_t count) {
  return syscall(SYSCALL_READ, fd, (uint32_t)buff, count);
}

void* sbrk (intptr_t increment) {
  return (void*)syscall(SYSCALL_SBRK, increment, 0, 0);
}
```

We also have the syscall function that's written in assembly:

```x86asm
[BITS 32]

global syscall

syscall:
    mov eax, [esp + 4]
    mov ebx, [esp + 8]
    mov ecx, [esp + 12]
    mov edx, [esp + 16]
    
    int 0x80
    ret
```

The arguments here are being read from the stack using the normal
32-bit C calling convention. At the point that `syscall`is entered, `[esp + 4]` 
contains the first argument, `[esp + 8]` the second, and so on. We move these
values into the registers that our kernel-side syscall handler expects and then trigger
interrupt `0x80`.

Before we move into the changes shown to the interrupt, it'll show
you the updated `process_t` ahead of time:

```c
typedef struct {
    void* buffer;
    uint32_t count;
    uint32_t size;
} process_reading_state_t;

typedef struct process {
    uintptr_t user_heap_end;
    uint32_t heap_pages_allocated;

    uint32_t wake_tick;
    process_reading_state_t reading_state;

    uint32_t pid;
    
    process_registers_t regs;

    process_states_t state;
    process_type_t type;

    struct process* next;

    page_directory_t* page_directory;

    void* kstack;
    void* ustack;
} __attribute__((packed)) process_t;
```

The first 2 pieces of data are for our `sbrk` calls where we track and
extend the heap, the `wake_tick` is for the sleep syscall and the
`reading_state` is for the read syscall.

At the end of `idt_init` we must add this line, this just adds the 0x80
interrupt to our idt as we have done before:

```c
    //software interrupts
    idt_set_gate(0x80, (uint32_t)syscall_entry, 0x08, 0xEE);
```

Notice that this gate uses `0xEE` rather than the `0x8E` we normally use for hardware 
interrupts. The important difference is the descriptor privilege level. Setting the DPL 
to 3 allows code running at user privilege to invoke this interrupt
with `int 0x80`. Without this, a user process would not be allowed to invoke the 
syscall interrupt directly.

Our entry is written as so:

```x86asm
syscall_entry:
    push dword 0x80
    push dword 0

    pusha 
    
    mov ax, ds
    push eax

    push esp

    call syscall_handler
    add esp, 4

    pop eax

    popa

    add esp, 8

    iret
```

The first two things that we push are used to fill in the error code and interrupt
number fields expected by our existing interrupt handling code.

The `syscall_handler` is the main meat and potatoes of the syscall
infrastructure:

```c
void syscall_handler(registers_t* regs) {
    int fd;
    uint32_t buffer;
    size_t count;
    switch (regs->eax) {
        case SYSCALL_EXIT:
            current_process->state = PROCESS_TERMINATED;
            context_switch(current_process, get_next_process(), regs);
            break;
        case SYSCALL_GETPID:
            regs->eax = current_process->pid;
            break;
        case SYSCALL_YIELD:
            context_switch(current_process, get_next_process(), regs);
            break;
        case SYSCALL_SLEEP:
            current_process->wake_tick = timer_get_ticks() + regs->ebx;
            current_process->state = PROCESS_SLEEPING;
            context_switch(current_process, get_next_process(), regs);
            break;
        case SYSCALL_WRITE:
            // a rather mock version of write syscall, only used for output to the terminal, will advance more later
            fd = regs->ebx;
            buffer = regs->ecx;
            count = regs->edx;

            
            if (fd != 1) {
                regs->eax = -1;
                break;
            }

            ((char*) buffer)[count] = '\0';
            vga_text_write(&terminal, (char*)buffer);
            
            regs->eax = (int)count;
            break;
        case SYSCALL_READ:
            // just like the previous, this is a mock, will do more when we get onto file system

            if (regs->ebx != 0) { 
              regs->eax = -1;
              return;
            }

            current_process->state = PROCESS_BLOCKED;
            current_process->reading_state.buffer = (void*)regs->ecx;
            current_process->reading_state.size  = regs->edx;
            current_process->reading_state.count = 0;
            context_switch(current_process, get_next_process(), regs);
            

            //keyboard is treated as stdin, so need to block until we recieve that data
            //need to block the process until we wait for input

            break;

        case SYSCALL_SBRK:
            current_process->user_heap_end += regs->ebx;
            
            //allocate more pages
            while (((current_process->user_heap_end + 4096) - USER_HEAP_START) / 4096 
                > current_process->heap_pages_allocated){
                map_page(current_process->page_directory,
                    USER_HEAP_START + (current_process->heap_pages_allocated++ * 4096),
                    (uintptr_t)alloc_frame(),
                    PAGE_PRESENT | PAGE_USER | PAGE_WRITABLE
                );
                    
            }

            regs->eax = current_process->user_heap_end;


            break;
        default:
            vga_text_writeline(&terminal, "syscall not found");
            break;
    }
    return;
}
```

`SYSCALL_EXIT`, `SYSCALL_GETPID` and `SYSCALL_YIELD` are all simple enough,
so let's look at the next couple and I'll explain them:

### `SYSCALL_SLEEP`

If you remember back to when we made our timer, we made a function to
get ticks, and we set what tick we should wake up on by adding the
current tick and the number passed to it. Then we perform a context switch. We
must then also update our timer handler to wake up a process after the
certain number of ticks has been reached:

```c
void timer_handler(registers_t* regs) {
    ticks++;
    if ((ticks % 100) == 0) {
        //vga_text_writeline(&terminal, " 1 second ");
    }

    process_t* traversal_process = process_head;
    while (traversal_process) {

        if (traversal_process->state == PROCESS_SLEEPING && ticks >= traversal_process->wake_tick) {
            traversal_process->state = PROCESS_READY;
        }
        traversal_process = traversal_process->next;
    }

    schedule(regs);
}
```

This code just traverses over the processes and checks if the current tick has 
reached or passed the wake tick. If it has, we wake the process up and set it to 
ready.

### `SYSCALL_WRITE`

As the comment states, it's a pretty mock version that writes based on the requested
size, as a `write` syscall typically does. After this function we can
basically set the privilege level of the VGA buffer back to 0 as we now have
a better way to write to VGA.

> **_NOTE:_** This implementation writes a null terminator at `buffer[count]`, so the
supplied buffer must have room for one extra byte. This is a simplification for 
terminal output implementation and is not how the general `write` syscall should be
implemented

> **_NOTE:_** An important limitation exists with this: the kernel
is directly de-referencing the user-provided buffer. A real operating system cannot simply
trust a pointer supplied by a user process, because the pointer could refer to an unmapped
address or to memory that the process should not be allowed to access. If you wish to develop
this OS further you would normally add validation or a safe user-memory access mechanism around this.
For now, I've kept my implementation simple.

### `SYSCALL_READ`

This is a combination of the previous writing and sleeping syscalls in
terms of functionality. Baically, we set the process to blocked and store 
the buffer, size and count in the `reading_state`. Then, like the
sleeping syscall, we traverse through the list of processes in `keyboard.c`, 
just as we did in `timer.c`:

```c
void keyboard_handler () {
    uint8_t scancode = inb(PS2_DATA);
    
    if (keyboard_modifier_keys(scancode)) {
        return;
    }
    if (scancode == EXTENDED_SCANCODE) {
        keyboard_extended_scancodes();
    }
    if (scancode & KEY_RELEASED) {
        return;
    }


    char c[2];
    c[0] = keymap[scancode];
    c[1] = '\0';

    switch (c[0]) {
        case '\n':
            vga_text_writeline(&terminal, "");
            break;
        case 27: 
            vga_text_clear(&terminal);
            break;
        case '\b':
            vga_text_backspace(&terminal);
            break;
        case '\t':
            vga_text_write(&terminal, "    ");
            break;
        default:
            if (shift_pressed) {
                c[0] = shift_keymap[scancode];
            }
            vga_text_write(&terminal, c);
            if (!c[0]) return;

            process_t* traversal_process = process_head;
            while (traversal_process) {
                if (traversal_process->state == PROCESS_BLOCKED && traversal_process->reading_state.size > 0) {
                    void* buffer = traversal_process->reading_state.buffer;
                    uint32_t size = traversal_process->reading_state.size;

                    //quite a crude way of doing this, (really only the context switcher should 
                    //be changing cr3), but it works
                    set_cr3((uintptr_t)traversal_process->page_directory);
                    ((char*)(buffer))[traversal_process->reading_state.count++] = c[0];
                    set_cr3((uintptr_t)kernel_directory);

                    if (traversal_process->reading_state.count >= size) {
                        traversal_process->reading_state.count = 0;
                        traversal_process->reading_state.size = 0;

                        traversal_process->state = PROCESS_READY;
                    }
                    
                }
                
                traversal_process = traversal_process->next;
            }
            break;
    }
}
```

All the changes are in the default case for the switch statement, as
you can see we just check if there is a character in c[0], as if this
wasn't checked, we could end up trying to use an invalid character value for keys
such as in `LGUI`.

For this block of code, we check each process if it's blocked and if
the size to take in is more than 0. Then we briefly change `CR3` so that the buffer
address refers to the blocked process's address space, allowing us to store
the current character in its buffer. Then if the
count has reached the size, then we reset the count and size and set the
process to ready.

> **_NOTE:_** Changing `CR3` here is somewhat dangerous. `CR3` controls the address space
currently being used by the CPU, so while it's temporarily set to another process's 
page directory, any memory access must be treated carefully. In a complete kernel,
this would normally be handled through a dedicated user-memory access mechanism rather than manually
switching `CR3` inside the keyboard handler.

### `SYSCALL_SBRK`

Before we can increase the heap, we must first create one, for that we
can just add this small piece of code to our `create_uprocess`:

```c
    map_page(new_process->page_directory,
            USER_HEAP_START,
            (uintptr_t)alloc_frame(),
            PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER
    );
    /* if you ever try to create more mappings and get a general protection fault
     * it is likely that you have overwritten an existing kernel mapping, as we copy
     * the kernel mappings to the user process for when we use our interrupts, just
     * be careful of this */

    new_process->user_heap_end = USER_HEAP_START + 4096;
    new_process->heap_pages_allocated = 1;
```

I just defined `USER_HEAP_START` in my mappings.h as `0x00A00000` which
is some free space. Now that we have a heap, we can then make our
function for growing it as defined in `SYSCALL_SBRK`.
In this syscall we add the increment to the end of the heap and then make sure
enough pages are mapped to cover the new heap size.
We then have a loop that iterates until enough pages have been allocated to cover
the heap end. The contents of this loop map each newly allocated page into
the heap.

For now, our implementation should also be though of as supporting heap growth rather than full
`sbrk` semantics. A negative increment would require us to shrink the heap and potentially
un-map and free pages, which we do not currently implement. We will ignore that case for now rather than trying
to handle it prematurely.

> **_NOTE:_** The page-count calculation here is deliberately simple, but be careful with
the boundary calculation. The number of mapped pages should be the number required to cover the range from
`USER_HEAP_START` up to `user_heap_end`. Adding an extra `4096` to `user_heap_end` can cause an unecessary
page to be allocated at page boundaries.

That's basically everything for our syscalls. The next stage will focus on making our user space
even more useful by implementing our own version of parts of the C standard library.
