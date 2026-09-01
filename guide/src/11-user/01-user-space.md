# Part XI: User Space

## What are we making

Now that processes are fully made, we want to take our processes and
make it possible for them to run outside of the kernel. While this
section is pretty distinct from tasking, we are still building directly
on top of it, and we will be editing the infrastructure for our tasking.
This section on making the user space is also split up into it\'s own
two parts:

-   Userspace: We get processes to run safely outside of the kernel
-   System calls: We allow user space processes to request services from
    the kernel

On x86 32 bit architecture, the cpu has 4 levels of privalege, but for
our OS we are only really using 2, these being Ring 0 privilege (for
kernel stuff) and and Ring 3 (for usermode stuff).

We know what ring we are in via the CPL (Current privilege level). This
is set from the currently loaded code segment, the CPL is the bottom 2
bits of the CS selector.\
So now we currently have:

    GDT
    ├── gdt_null
    ├── gdt_code
    └── gdt_data

And next we will have:

    GDT
    ├── gdt_null
    ├── gdt_code
    ├── gdt_data
    ├── gdt_user_code
    └── gdt_user_data

With the user data and code segments having their appropriate flags. The
privilege level is stored inside the GDT descriptor if you remember
correctly when we made our GDT. So how we set our ring level is by
changing which GDT descriptor is loaded into CS

### Rings are NOT enough

If we set the ring level, our memory is still not completely protected,
this is becuase we also need to set priveleges within the page tables.
As in our OS we have flags for present, writable and user. Anything to
be directly accessed by the userspace must be set with the USER flag,
otherwise the user space process will not be able to access it and a
page fault will happen. We will also need to edit our VMM a bit, as when
we create our processes we will need to edit page directories other than
the one that we are currently in.

Each user process will have it\'s own page directory, this is becuase
for each process we will probably have the same start address for each
program, for example `0x00400000` we want a constant address to mark the
start but will actually reference different locations in physical
memory.

As well as changing the GDT and paging, we need ot do a couple of other
changes in order to make our architecture:

-   Changing processes so that each user one gets it\'s own page
    directory
-   Giving each process it\'s own user stack in it\'s user address space
-   Needs a safe to execute kernel stack in rder to temporarily enter
    the kernel when it needs to
-   Copying compiled code to an address marked as ring 3

We also need to know about TSS (Task State Segment). This is a
CPU-defined structure that tells the CPU which kernel stack we should
use if we go from ring 0 to ring 3.

### Ring 3 Execution

We can\'t just call user functions, the perfect place to change things
like CS in order to enter a user process is the scheduler that we have
made before, so we will also have to edit this aswell. There is also the
fact that the code we jump into for the task has to belong to a page
that is assigned as a user.

This is all we need to know to make user space for now, it includes
refactoring of alot of our previous code, and really we could have been
implementing the user space parts from the very start, but it would have
taken until this stage in order to actually get user space processes
working, as we would still need to implement system calls after this. We
will start by editing our GDT and making Ring 3.

## GDT change

I will not explain the data that we put inside of the new GDT entries,
as each bit has already been explained prior, but here is the new
structure for our GDT:

``` language-x86asm
global gdt_tss

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
gdt_user_code:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0xFA
    db 0xCF
    db 0x00
gdt_user_data:
    dw 0xFFF
    dw 0x0000
    db 0x00
    db 0xF2
    db 0xCF
    db 0x00
gdt_tss: ; will be populated in C later
    dw 0
    dw 0
    db 0
    db 0
    db 0
    db 0
gdt_end:
```

## Process creation

I have also implemented this new structure for the processes:

    typedef enum {
        PROCESS_RUNNING = 0,
        PROCESS_READY = 1,
        PROCESS_BLOCKED = 2,
        PROCESS_SLEEPING = 3,
        PROCESS_TERMINATED = 4
    } process_states_t;

    typedef struct {
        uint32_t ss;
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

    } __attribute__((packed)) process_registers_t;

    typedef enum {
        PROCESS_KERNEL,
        PROCESS_USER
    } process_type_t;

    typedef struct process {
        uint32_t pid;
        
        process_registers_t regs;

        process_states_t state;
        process_type_t type;

        struct process* next;

        uintptr_t* page_directory;

        void* kstack;
        void* ustack;
    } __attribute__((packed)) process_t; 

This is basically a refactor of the kernel process, there are multiple
ways we could have done this, but this will just be the simplest to
allow all of our process list to have the same type.\
This change will include alot of refactoring of our code, as i said
before we should have probably considered the eventual development of
our user space earlier on, but here we are! (and it\'s pretty important
to be okay refactoring stuff in programming in general).

I\'ll allow you to change all of the current existing code in procman.c
to use process_t instead of kprocess_t and also kstack instead of
\"stack\". After refactoring, let\'s take a look at our the changes made
to the creation of processes

    #define USER_STACK_TOP 0xBFFFF000

    process_t* create_process(void* task_address, process_type_t type);
    void create_kprocess(process_t* new_process);
    void create_uprocess(process_t* new_process);

As you can probably guess the create_process function will handle all
generic process stuff, and then our respective user space and kernel
space processes will do everything that a user and kernel space process
would require.

    process_t* create_process(void* task_address, process_type_t type) {
        process_t* new_process = kmalloc(sizeof(process_t));

        new_process->type = type;
        new_process->pid = next_pid++;
        new_process->state = PROCESS_READY;
        new_process->kstack = kmalloc(KERNEL_STACK_SIZE);
        new_process->next = NULL;

        new_process->regs.eax = 0;
        new_process->regs.ebx = 0;
        new_process->regs.ecx = 0;
        new_process->regs.edx = 0;
        new_process->regs.esi = 0;
        new_process->regs.edi = 0;
        new_process->regs.ebp = 0;

        new_process->regs.eip = (uintptr_t)task_address;
        //esp defined based on type
        
        new_process->regs.eflags = 0x202;

        if (type == PROCESS_KERNEL) {
            create_kprocess(new_process);
        } else {
            create_uprocess(new_process);
        }

        process_t* traversal_process = process_head;
        while (traversal_process) {
            if (!traversal_process->next) {
                traversal_process->next = new_process;
                break;
            }
            traversal_process = traversal_process->next;
        }
        
        return new_process;
    }

    void create_kprocess(process_t* new_process) {
        new_process->regs.esp = (uint32_t)(new_process->kstack) + KERNEL_STACK_SIZE;
        new_process->regs.cs = 0x08;
        new_process->regs.ds = 0x10;
        new_process->regs.ss = 0x10;

        new_process->page_directory = kernel_directory;
    }

    void create_uprocess(process_t* new_process) {
        new_process->regs.esp = USER_STACK_TOP;
        
        new_process->regs.cs = 0x18 | 3;
        new_process->regs.ds = 0x20 | 3;
        new_process->regs.ss = 0x20 | 3;

        new_process->page_directory = (page_directory_t*)alloc_frame();
        memset((page_directory_t*)new_process->page_directory, 0, 4096);

        for (uint32_t i = 0; i < 1024; i++) {
            (*new_process->page_directory)[i] = (*kernel_directory)[i];
        }

        uintptr_t ustack_frame = (uintptr_t)alloc_frame();

        map_page(new_process->page_directory, 
                USER_STACK_TOP - 4096,
                ustack_frame,
                PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER
        );

        new_process->ustack = (void*)(USER_STACK_TOP - 4096);
    }

what do kernel processes specifically require in creation?

not much, for a kernel space process we just need to set it with the cs
and ds registers that we used before, this however includes the addition
of the ss register (which is just set to the same as ds). We also set
the kernel stack and page directory the same way we did before

what do user processes specifically require in creation?

We of course set esp to the top of the user stack (remember that the
stack grows downward). And set the CS, DS and SS The value in hex is
just the offset in our GDT table that points to user code and user data.
The number 3 turns on the bottom two bits of the segment selectors which
sets the CPL.\
Now it\'s time to handle the process\'s memory: We allocate a frame and
clear it\'s memory and this is used for the page directory. Now the next
step may confuse you a little, but this is an important step. What we do
is we copy the kernel\'s identity mapping to the page directory of the
user process. This is important becuase user processes will still need
to access kernel resources, an example of this is being syscalls (the
next chapter) where we will need to execute kernel code via an interrupt
that ring 3 can access. We then allocate a frame for the user space
stack (that is 4Kib long), map it, and then set it\'s address in the
process

### What about destruction

        if (proc->type == PROCESS_USER) {
            unmap_page(proc->page_directory, USER_STACK_TOP - 4096);
            unmap_page(proc->page_directory, USER_CODE_BASE);
            unmap_page(proc->page_directory, USER_VGA);
            unmap_page(kernel_directory, (uintptr_t)(proc->page_directory));
        }

Simple enough, just a little more memory to unmap and they we are good.

## Changes in the context switcher

The code is short enough to view all of the changes, so i\'ll just paste
it all here:

    #include "context.h"
    volatile bool return_to_user;

    void save_context(process_t* process, registers_t* regs) {
        process->regs.eip = regs->eip;
        process->regs.cs = regs->cs;
        process->regs.eflags = regs->eflags;
        process->regs.ds = regs->ds;

        process->regs.edi = regs->edi;
        process->regs.esi = regs->esi;
        process->regs.ebp = regs->ebp;
        process->regs.ebx = regs->ebx;
        process->regs.edx = regs->edx;
        process->regs.ecx = regs->ecx;
        process->regs.eax = regs->eax;
        
        if (process->type == PROCESS_KERNEL) {
            process->regs.esp = regs->esp;
        } else {
            process->regs.esp = regs->user_esp;
            process->regs.ss = regs->ss;
        }
    }

    void load_context(process_t* process, registers_t* regs) {
        regs->eip = process->regs.eip;
        regs->cs = process->regs.cs;
        regs->eflags = process->regs.eflags;
        regs->ds = process->regs.ds;

        regs->edi = process->regs.edi;
        regs->esi = process->regs.esi;
        regs->ebp = process->regs.ebp;
        regs->ebx = process->regs.ebx;
        regs->edx = process->regs.edx;
        regs->ecx = process->regs.ecx;
        regs->eax = process->regs.eax;

        if (process->type == PROCESS_KERNEL) {
            regs->esp = process->regs.esp;
        } else {
            regs->user_esp = process->regs.esp;
            regs->ss = process->regs.ss;
        }
    }

    void context_switch(process_t* old_process, process_t* new_process, registers_t* regs) {
        current_process = new_process;
        if (old_process->state == PROCESS_RUNNING) {
            old_process->state = PROCESS_READY;
        } 
        
        save_context(old_process, regs);
        new_process->state = PROCESS_RUNNING;

        return_to_user = (new_process->type == PROCESS_USER);

        if (new_process->type == PROCESS_USER) {  
            tss.esp0 = (uintptr_t)(new_process->kstack) + KERNEL_STACK_SIZE;
            set_cr3((uintptr_t)new_process->page_directory);
        } else if (new_process->type == PROCESS_KERNEL) {
            set_cr3((uintptr_t)kernel_directory);
        }

        if (old_process->state == PROCESS_TERMINATED) {
            destroy_process(old_process);    
        }

        load_context(new_process, regs);
    }

For user and kernel processes, the only thing that changes is in regards
to saving and loading the stack. The stack segment doesn\'t really
matter for kernel processes, so we don\'t really have to do anything
with that afaik.

For the context switching function we add in some new logic that
terminates the old process if it is marked as TERMINATED. This is going
to be important later in this section when an exception happens in a
user process. Another thing we do for user and kernel processes is
setting cr3 appropriately and then we do another thing which is setting
the esp0 value of the TSS. Let\'s cover the TSS now.

### TSS creation and editing

Here\'s the information that would be required in the header file for
the process manager:

    typedef struct {
        uint32_t prev_tss;

        uint32_t esp0;
        uint32_t ss0;

        uint32_t esp1;
        uint32_t ss1;

        uint32_t esp2;
        uint32_t ss2;

        uint32_t cr3;
        uint32_t eip;
        uint32_t eflags;

        uint32_t eax;
        uint32_t ecx;
        uint32_t edx;
        uint32_t ebx;

        uint32_t esp;
        uint32_t ebp;
        uint32_t esi;
        uint32_t edi;

        uint32_t es;
        uint32_t cs;
        uint32_t ss;
        uint32_t ds;
        uint32_t fs;
        uint32_t gs;

        uint32_t ldt;

        uint16_t trap;
        uint16_t iomap_base;
    } __attribute__((packed)) tss_t;

    extern tss_t tss;
    extern uint8_t gdt_tss[];

    void init_tss(void);
    extern void load_tss(void);

As i said before, TSS just defines where we need to go to get to the
kernel stack (for our purposes), so we only really care about 2
registers defined in our structure, these being esp0 and ss0, we can
forget about everything else. `gdt_tss[]` is just the a reference to the
gdt descriptor for the TSS that we made global. Let\'s look at the
implementation code:

    tss_t tss;

    void init_tss(void) {
        uintptr_t base = (uintptr_t)&tss;
        uint32_t limit = sizeof(tss_t) - 1;
        gdt_tss[0] = limit & 0xFF;
        gdt_tss[1] = (limit >> 8) & 0xFF;

        gdt_tss[2] = base & 0xFF;
        gdt_tss[3] = (base >> 8) & 0xFF;

        gdt_tss[4] = (base >> 16) & 0xFF;
        gdt_tss[5] = 0x89;

        gdt_tss[6] = (limit >> 16) & 0x0F;
        gdt_tss[7] = (limit >> 24) & 0xFF;

        memset(&tss, 0, sizeof(tss_t));

        tss.ss0 = 0x10;
        tss.esp0 = 0;

        tss.iomap_base = sizeof(tss_t);
        load_tss();
    }

And then we have some assembly:

``` language-x86asm
[BITS 32]

global load_tss

load_tss:
    mov ax, 0x28
    ltr ax
    ret
```

First we define a global variable for the tss. Next his handling the
data to be stored in the gdt descriptor. The base is set to the address
of the tss and the limit is set to the size of the tss. I have made
setting the descriptor similar to the structure we had in our assembly,
just so you can see what the data means and compare it to the info that
i gave about gdt entries if you so wish to see what the data
individually means. But basically this is just a bunch of data that
tells the CPU where the TSS is and how big it is.

Next we set the ss0 of the tss to 0x10, this is used to change the
segment selector when we want to load the stack from a lower privelege
level to a higher one. esp0 does the same, but instead changes the stack
pointer instead of the segment selector, so this will need to be changed
each time we load a new user process, this is what we did in the context
switcher as seen before.

For the assembly, ltr is an instruction that just means \"load task
register\", the offset of the tss descriptor is given to it. This is all
we need for the TSS setup.

### Interrupts

As we have the introduction of user_esp and ss registers to our register
type, we need to introduce these to the type that the interrupt uses:

    /* registers passed from asm to C */
    typedef struct {
        uint32_t ds;

        uint32_t edi;
        uint32_t esi;
        uint32_t ebp;
        uint32_t esp;
        uint32_t ebx;
        uint32_t edx;
        uint32_t ecx;
        uint32_t eax;

        uint32_t interrupt_number;
        uint32_t error_code;

        uint32_t eip;
        uint32_t cs;
        uint32_t eflags;

        uint32_t user_esp;
        uint32_t ss;
    } __attribute__((packed)) registers_t;

With this structure, iret will handle the user_esp and ss on it\'s own,
so don\'t worry

### mapping changes

As i said before, we will now have multiple page directories; we will
need to change our user space page directories from kernel processes, i
already showed you the use of this refactored function before if you
noticed within the create_uprocess function. This isn\'t a hard change,
instead of universally using the current_directory within all of our
mapping and translation functions, we just use a pointer to a directory
that we pass to all of the functions. Remember that the kernel uses
identity mapping, so we don\'t have to consider the differences between
virtual memory and physical, as the kernel technically still uses
phisical memory addresses.

    //DIRECTORY MUST BE A VIRTUAL ADDRESS, identity mapped for kernel 
    void map_page(page_directory_t* directory, uintptr_t virtual_address, uintptr_t physical_address, uint32_t flags) {
        uint16_t dir_index = (virtual_address >> 22);
        uint16_t table_index = (virtual_address >> 12 & 0x3FF); 

        uint32_t dir_entry = (*directory)[dir_index];
        page_table_t* selected_table;
        if (!(dir_entry & PAGE_PRESENT)) {
            selected_table = create_page_table(directory, dir_index, flags);
        } else {
            selected_table = (page_table_t*)(dir_entry & 0xFFFFF000);
        }

        (*selected_table)[table_index] = physical_address | flags;
        if (directory == current_directory) {
            flush_tlb_page(virtual_address);
        }
    }

    void unmap_page(page_directory_t* directory, uintptr_t virtual_address) {
        uint16_t dir_index = (virtual_address >> 22);
        uint16_t table_index = (virtual_address >> 12 & 0x3FF); 

        uint32_t dir_entry = (*directory)[dir_index];
        page_table_t* selected_table = (page_table_t*)(dir_entry & 0xFFFFF000);
        (*selected_table)[table_index] &= ~(PAGE_PRESENT);
        flush_tlb_page(virtual_address);
    }

    uintptr_t get_physical_address(page_directory_t* directory, uintptr_t virtual_address) {
        uint16_t dir_index = (virtual_address >> 22);
        uint16_t table_index = (virtual_address >> 12 & 0x3FF); 

        uint32_t dir_entry = (*directory)[dir_index];
        page_table_t* selected_table = (page_table_t*)(dir_entry & 0xFFFFF000);
        return (*selected_table)[table_index];
    }

### writing our user code

Let\'s take a small break from writing the kernel, and write our code
that our first userspace process will have.

    #include 
    void _start(void)
    {
        volatile uint32_t *bad_address = (uint32_t *)0xDEADBEEF;

        *bad_address = 1234;

        volatile unsigned short* vga = (unsigned short*)0x00B00000;

        vga[0] = 'U' | (0x07 << 8);

        while (1) {
        }
    }

this code has 2 tests. The first one will give a page fault at an
address that either should not exist or we will not have access too, the
second is directly writing to the VGA, which will indicate that we are
actually able to do something functional.

Along side our user space code, we must have a linker to be used, this
is becauase how we load our code will be by copying the data in our
binary directly to a space in memory that we can allocate as a user
page. Here is the linker:

    ENTRY(_start)

    SECTIONS
    {
        . = 0x00400000;

        .text :
        {
            *(.text)
        }

        .rodata :
        {
            *(.rodata)
        }

        .data :
        {
            *(.data)
        }

        .bss :
        {
            *(.bss)
        }
    }

Just like a simple linker like we made for our original kernel, and then
we must build a specific way for a makefile. My makefile skills are
pretty poor, so i\'ll just put the commands here:

     #user tests
        $(CC) -g -m32 -ffreestanding -fno-pie -fno-pic -c $(USER_TEST_FILE_C) -o user_test.o
        ld -m elf_i386 -T $(USER_LINKER) user_test.o -o user_test.elf
        objcopy -O binary user_test.elf user_test.bin
        objcopy -I binary -O elf32-i386 -B i386 user_test.bin user_test_binary.o

        ld -m elf_i386 -T $(LINKER) kernel.o kernel_main.o vga.o interruptc.o interrupta.o timer.o kb.o pmm.o vmm.o vmma.o heap.o tasks.o context.o scheduler.o tasksa.o user_test_binary.o -o kernel.elf

our linker will generate a label for the start and end of our code,
which we can then load into an address that will be for user space.

### Kernel code

Add this to the end of our kernel main code:

        //userspace testing

        void* code_frame = alloc_frame();

        extern unsigned char _binary_user_test_bin_start[];
        extern unsigned char _binary_user_test_bin_end[];

        //copy user program to physical frame
        uintptr_t user_size = (uintptr_t)(_binary_user_test_bin_end - 
                _binary_user_test_bin_start);

        for (uintptr_t i = 0; i < user_size; i++) {
            ((uint8_t*)code_frame)[i] = _binary_user_test_bin_start[i];
        }

        process_t* user_proc = create_process((void*)USER_CODE_BASE, PROCESS_USER);

        map_page(
            user_proc->page_directory,
            USER_CODE_BASE, 
            (uintptr_t)code_frame, 
            PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER
        );

        //map vga so process ring 3 can access
        map_page(
            user_proc->page_directory,
            USER_VGA,
            0xB8000,
            PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER
        );

        for (;;);
    }

This is everything we described before, we copy the user space code into
an allocated frame and then create a process and map it as a user, you
may be wrondering where the USER_CODE_BASE definition is, and i actually
created a seperate header file for this:

    #ifndef MAPPINGS_H
    #define MAPPINGS_H

    #define USER_CODE_BASE 0x00400000
    #define USER_VGA 0x00B00000
    #define HEAP_START 0xC0000000 


    #endif

This is because when we previously made the kernel heap allocator, we
set it\'s address as 0x00400000. It is important to make a file of these
mapping so that they don\'t collide, if they did it would cause
exceptions and would not be very good. This is due to the kernel
mappings being included within all of our user space processes, so we
can\'t map any thing for the userspace over anything that already exists
for the kernel.

After this, if we then run our kernel and we get the DEADBEEF exception
then everything is working, but there is one issue that we currently
haven\'t implemented, this is the fact that a user exception crashes the
operating system (this is seen by us no longer being able to write
text).

### Fixing exception

To change this, let\'s just change our isr handler a little:

    void isr_handler(registers_t* regs) {
        switch (regs->interrupt_number) {
            case 14:
                page_fault_handler(regs);
                break;
            default:
                vga_text_writeline(&terminal, exception_messages[regs->interrupt_number]);
                break;
        }

        if ((regs->cs & 3) == 3) {
            current_process->state = PROCESS_TERMINATED;
            context_switch(current_process, get_next_process(), regs);
            return;
        }


        for (;;);
    }

And that should be our userspace code done. We can cause exceptions and
our OS should still run, and we should be able to write to the VGA. The
next step is to make the userspace actually usable, by implementing
syscalls, which allow us to use kernel resources from our userspace.

\-\-- DEV STUFF \-\-\-- remember changes in process creation. \--
context switcher changes too. \-- and then interrupt with retrieving ss.
\-- creation of TSS \-- user programs compiled seperately \-- need to
change map_page to take directory (and all other functions) \-- write
about destruction functions and exception handling \--

