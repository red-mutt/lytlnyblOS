# Part X : Processes and Multitasking

## A little word on the next 3 chapters

The previous 3 chapters we had were each making 3 major components of
memory management. These next 3 parts are going to be formatted in the
same way but instead of making memory management we are making the
process management subsystem. Currently, our kernel just makes the CPU
execute one instruction after another from one stack, there is no such
thing as multitasking and running multiple things at the same time,
this is what we want to create.

After making tasking the CPU still executes one instruction at a time,
but the tasking system continually swaps between different execution
contexts quick enough to the point where they seem to run
simultaneously.

### What is a process?

A process is an instance of a computer program that is being executed by
the operating system, it would simply contain everything the CPU needs
to continue executing some code later. For instance a process
must remember:

-   Which instruction was being executed (`EIP`)
-   The current stack (`ESP`)
-   The base pointer (`EBP`)
-   General registers
-   flags register
-   which address space it owns (`CR3`)
-   Process state
-   Process ID
-   Kernel stack

Tasking is simply just a loop that goes like:
` Run A -> Save A -> Load B -> Run B -> Save B -> ...`

The process management subsystem is split into these 3 parts:

1.  Process Management: Responsible for creating and storing processes.
2.  Context Switching: The hardest part, this is responsible for
    changing which process the CPU is executing.
3.  Scheduler: The scheduler decides which process should run next and
    uses the context switcher to make the switch.

## What we are making now

As we said before, a process is an **instance**, it's not the program
itself, it's just a saved execution state that contains the data we
need to remember to continue execution.

### The process structure

For our process structure we simply just need to define a `process_t`
that contains all of our data somewhere in kernel memory. Let's look at
that info in detail:

1.  Identity (PID): we need to identify what process is what, this is
    why we have the PID, this is simply just a number. For example
    `PID = 5` means process number 5.
2.  CPU state: The CPU has its registers like:
    `EAX, EBX, ECX, EDX, ESP, EBP, EIP and EFLAGS` that describe what
    the CPU is doing. If we switch away from a task, we need to store
    these values for when we return.
3.  Page Directory: A process must have a pointer to its page
    directory, when the process runs, the page directory must be loaded
    into CR3.
4.  Stack: Each process needs a stack, so it would have its own stack
    pointer that we store.
5.  State: This contains what is happening with the process, for example,
    the state can say that it's running etc
6.  Linking to other processes: The kernel needs a way to find all
    processes, so each process can point to the next one, and then if we
    need to store multiple we can store them as a linked list.

### Process lifetime

After creating structure, we need to make code that manages these
structures. Each process has a lifecycle, Creating a process is made by
simply assigning the next available PID, mapping a page for the stack,
setting the initial CPU state (as the process has never run before). And
then it must be added to the process list.

The process list is just the way the kernel stores every process that
exists, just like before with the heap, we use a linked list,
you have freedom here, you can use an array if you want, you have
some issues deciding how big it should be, what happens when it fills,
and how entries are removed.

After being added to the process list we can make some functions for
process lookup to find a process by PID. When we delete a process we
just delete it from the process list by skipping over it. You would also
then need to free its memory by freeing its stack and destroying the
page directory for user processes. It's an important distinction that
we are currently **ONLY** making kernel processes, these would use the
kernel page directory and hence wouldn't have their page directory be
made free.

For **loading** this will be done by the context switcher, so we don't
really need to do this right about now.

### THE KERNEL IS RUNNING!?!?

The kernel, at this stage in or OS is running an infinite loop, how do
we take this already running code and build it into our tasking
structure, to cope with this, when we initialize the process manager, we
must immediately create a `process_t` representing the current kernel
execution. Later, when we make the context switcher, we will already
have somewhere to save the kernel's registers.

This is pretty much all we need to know to create this stage. Let's get
making.

## The code

### The header

```c
#ifndef PROCMAN_H
#define PROCMAN_H

#include 
#include 

#include "../kernel/interrupts.h"
#include "../memory/vmm.h"
#include "../memory/pmm.h"

#define INITIAL_PID 1
#define KERNEL_STACK_SIZE 0x4000

extern uint8_t kernel_stack_bottom;

typedef enum {
    PROCESS_RUNNING = 0,
    PROCESS_READY = 1,
    PROCESS_BLOCKED = 2,
    PROCESS_SLEEPING = 3,
    PROCESS_TERMINATED = 4
} process_states_t;

typedef struct {
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t ds;

    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

} kprocess_registers_t;

typedef struct kprocess {
    uint32_t pid;
    kprocess_registers_t regs;
    process_states_t state;
    struct kprocess* next;
    page_directory_t* page_directory;
    void* stack;
} kprocess_t; 

void init_procman();

kprocess_t* create_kprocess(void* task_address);

void destroy_kprocess(kprocess_t* proc);

kprocess_t* find_process_by_pid(uint32_t pid);

#endif
```

The first thing you might take note of here is that the initial PID we
use has a value of 1. This is because PID 0, often has a special meaning
in OS design. On Linux this refers to an idle/swapper process, for now
in our OS this will just be an invalid PID, but later we could use this
to refer to an idle process or something.

Next we define the size of the stack for kernel processes as 16Kib,
after this we have an enumerator for all of our process states, let's
look at them:

-   `PROCESS_RUNNING`: The process is currently executing on the CPU.
-   `PROCESS_READY`: The process could run at this moment, but isn't
    because the CPU is handling another process
-   `PROCESS_BLOCKED`: The process cannot currently continue because 
    it's waiting for some event or resources
-   `PROCESS_SLEEPING`: Similar to blocked, but the reason is primarily
    because of time
-   `PROCESS_TERMINATED`: The process has finished execution and should no
    longer be scheduled

For the registers data structure it's a lot similar to the one we had
with our interrupts, but instead we removed things that are useless like
the error code and interrupt number.

For the data structure for the processes, we simply have the PID, the
registers which isn't a pointer but the data itself is embedded, the
state of the process a pointer to the next process, a pointer to the
page director and finally a pointer to the stack.

Our functions then are pretty simple, we have initialization, creation,
destruction and searching.

The last thing I haven't mentioned is the `kernel_stack_bottom` as an
external variable, for this we need to look all the way back when we moved
our kernel into protected mode:

```x86asm
global kernel_stack_bottom
global kernel_stack_top

; no org code starts at 0x0900 though
[bits 16]
start:
    mov ax, cs
    mov ds, ax

    mov si, hello_string - start
    call print_string

    jmp enter_protected

print_string:
    mov ah, 0Eh

print_char:
    lodsb ; sets al = [DS:SI++]

    cmp al, 0
    je done
    
    int 10h

    jmp print_char

done:
    ret

enter_protected:
    cli ;disable interrupts
    lgdt [gdtr - start] ; load GDT registor with start address of GDT
    mov eax, cr0
    or eax, 1 ;set protection enable bit in control register 0 (cr0)
    mov cr0, eax

    ; perform far jump to selector 08h (offset into GDT, pointing at a 32bit
    ; PM code segment descriptor)
    ; to load CS with proper PM32 descriptor)


    CODE_SEG equ gdt_code - gdt_start
    jmp CODE_SEG:p_mode_main
[bits 32]
p_mode_main:
    mov ax, 10h
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, kernel_stack_top

    mov byte [0xB8000], 'P'
    mov byte [0xB8001], 0x02

    ; go into C

    extern kernel_main
    call kernel_main
hang:
    hlt
    jmp hang

hello_string db 'Hello World!, i am lytlnyblOS, in real mode', 0

gdt_start:
gdt_null:
    dq 0
gdt_code:
    dw 0xFFFF ; limit
    dw 0x0000 ; base_low
    db 0x00 ;base_middle
    db 0x9A ;access
    db 0xCF ;flags + limit high 4 bits
    db 0x00 ;base_high
gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92
    db 0xCF
    db 0x00
gdt_end:
gdtr:
    dw gdt_end - gdt_start - 1 ; set manually for testing
    dd gdt_start

section .bss 
align 16
kernel_stack_bottom:
    resb 0x4000
kernel_stack_top:
```

There are 2 things we have changed here. The first thing that has
changed is in the `p_mode_main` label. We change the value that we move
into esp from `0x9000` to `kernel_stack_top`. At the bottom we then have
another addition, this is just memory that we reserved for the main
kernel stack. As before when we just set `ESP` as `0x9000`, the stack
didn't have a defined size. We can then later use the
`kernel_stack_bottom` to make a pointer to the bottom of the kernel stack.

### Implementation

```c
#include "procman.h"
#include "../memory/heap.h"

kprocess_t* process_head;
kprocess_t* current_process;
uint32_t next_pid;

void init_procman() {
    process_head = NULL;
    next_pid = INITIAL_PID;


    kprocess_t* kernel_process = kmalloc(sizeof(kprocess_t));
    kernel_process->pid = next_pid++;
    kernel_process->state = PROCESS_RUNNING;
    kernel_process->page_directory = get_current_directory();
    
    kprocess_registers_t kernel_regs = {0};

    kernel_process->regs = kernel_regs;
    kernel_process->next = NULL;

    kernel_process->stack = (void *)&kernel_stack_bottom;

    process_head = kernel_process;
    current_process = kernel_process;
}

kprocess_t* create_kprocess(void* task_address) {
    kprocess_t* new_process = kmalloc(sizeof(kprocess_t));
    new_process->pid = next_pid++;
    new_process->state = PROCESS_READY;
    new_process->stack = kmalloc(KERNEL_STACK_SIZE);
    new_process->next = NULL;
    
    new_process->regs.eax = 0;
    new_process->regs.ebx = 0;
    new_process->regs.ecx = 0;
    new_process->regs.edx = 0;
    new_process->regs.esi = 0;
    new_process->regs.edi = 0;
    new_process->regs.ebp = 0;

    new_process->regs.eip = (uintptr_t)task_address;
    new_process->regs.esp = (uint32_t)(new_process->stack) + KERNEL_STACK_SIZE;
    //set these to the gdt_code and gdt_data back in the first ASM file.
    new_process->regs.cs = 0x08;
    new_process->regs.ds = 0x10;
    //sensible eflags value
    new_process->regs.eflags = 0x202;

    new_process->page_directory = kernel_directory;

    kprocess_t* traversal_process = process_head;
    while (traversal_process) {
        if (!traversal_process->next) {
            traversal_process->next = new_process;
            break;
        }
        traversal_process = traversal_process->next;
    }
    return new_process;
}

void destroy_kprocess(kprocess_t *proc) {
    if (!proc || proc->state == PROCESS_RUNNING) return;

    kprocess_t* traversal_process = process_head;
    if (!traversal_process->next && traversal_process->pid == proc->pid) {
        process_head = traversal_process->next;
        kfree(proc->stack);
        kfree(proc);
        return;
    }

    while (traversal_process) {
        if (!traversal_process->next) {
            traversal_process = NULL;
            break;
        }

        if (traversal_process->next->pid == proc->pid) {
            traversal_process->next = traversal_process->next->next;
            break;
        } 
        traversal_process = traversal_process->next;
    }

    if (!traversal_process) {
        return;
    }

    kfree(proc->stack);
    kfree(proc);
}

kprocess_t* find_process_by_pid(uint32_t pid) {
    kprocess_t* traversal_process = process_head;
    while (traversal_process) {
        if (traversal_process->pid == pid) {
            return traversal_process;
        } 
        traversal_process = traversal_process->next;
    }
    return NULL;
}
```

Global variables are simple, `process_head` is the head of the process
list, `current_process` is for the currently running process, and the next
PID is for the next assignable PID.

### `init_procman`

First we initialize the `process_head` to `NULL` and the `next_pid` to 1.
The next steps are building our main `kernel_process` sufficiently. We
set the PID to 1, set the process to running, and use
`get_current_directory()` (a new function we will make) to get the page
directory that is currently used (which is the kernel directory).

Next all our registers are set to 0, this is because the CPU state will
change when we change processes using the context switcher. And then we
set the next process to NULL too. Next the stack pointer is set to a
pointer of the bottom of the kernel stack. And then we set the
`current_process` and `process_head` accordingly.

### `create_kprocess`

The point of this function is not to create a process exactly how we
want it, but to create a base that we can use later. First we use
`kmalloc` to allocate some memory for the process and store it in
the heap. Next we then store the PID and state accordingly.

The 16Kib stack can then also be allocated using the heap too. When we
implement user processes we will probably want to map the pages our self
which will give us more control over the address spaces, as we have to
control page permissions and stack size.

We then set general purpose registers to 0, set `EIP` to a pointer to the
task address (which is given to the function). And esp is set to the top
of the stack (as stack grows downward in memory). CS and DS are then set
to the code and data segments that we created back when we created our
GDT. We then give a sensible `EFLAGS` value and set the page directory
to the `kernel_directory` which can be made public by including:
`extern page_directory_t* kernel_directory;` in the header for the VMM.

Finally, we traverse the linked list and add the newly created process
to the end.

### `destroy_kprocess`

For this function, if the process is null or if it's running then we
return without doing anything.
If the targeted process is first in the memory we skip over it and free
the process's stack and the process itself, and then return.

If it's not the first in the list we then traverse and if we can find
it we skip over it. And then free the stack and process. If not found,
we simply just return without freeing anything.

### `find_process_by_pid`

A simple function. We just traverse until the PID matches and then
return, if we don't find anything, we just return NULL.

## Testing

```c
    kprocess_t* p1 = create_kprocess(NULL);
    kprocess_t* p2 = create_kprocess(NULL);

    vga_text_write(&terminal, "PIDs: ");
    vga_text_write_hex(&terminal, p1->pid);
    vga_text_write(&terminal, " ");
    vga_text_write_hex(&terminal, p2->pid);
    vga_text_writeline(&terminal, "");

    if (find_process_by_pid(p1->pid) == p1 && find_process_by_pid(p2->pid) == p2) {
        vga_text_writeline(&terminal, "CREATE/LOOKUP OK");
    }

    destroy_kprocess(p1);

    if (find_process_by_pid(p1->pid) == NULL) {
        vga_text_writeline(&terminal, "DESTROY OK");
    }
```

Here's some simple code that we can put at the end of main to
test our processes, this should print out that everything is working.
And now we can move onto context switching.

