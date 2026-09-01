# Paging and Virtual Memory

## Some context

Now that we have already conquered the Physical Memory Manager (PMM) and
we have our 4096 byte physical frames. Now we must fufill the VMM\'s job
which is to make software (which is currently only our kernel) into
thinking it has a contiguous and private block in memory, when in
reality it is divided up into 4KiB frames. This is done by the CPU\'s
memory managment unit (MMU) which is a physical piece of hardware (So we
are back to writing drivers).

Currently the CPU treats all adresses as physical, but after this
section all addresses will be treated as a virtual address. A virtual
address is simply just a fake address that the CPU trnaslates into a
physical one. Currently we already have a pretty simple paging system,
this is where the virtual address = the physical address.

## Why do we want to implement this

There are a couple reasons why we would want to implement paging on top
of our current system. The first is process isolation. When we go onto
implement processes one process may use one frame and the other may use
the next, but process A can still overwrite data in Process B, a modern
operating system wouuld not allow something like this.

Another issue is that programs will assume fixed addresses, this means
that if we define `int x;` the compiler defines it\'s location as a hard
codes place in memory. However, with paging the variable\'s location is
relative to the location of the program as we have our virtual memory
addresses.

So the reason we make this is really only for infrastructure for future
feature like processes, usermode and the heap. But right now with our
kernel implementing paging will also allow for a form of memory
protection where if we were to do an action that would corrupt memory it
would instead cause a page fault.

## How x86 Paging works

After we implement paging each CPU-generated adress is now a virtual
address, so it implements itself pretty silently. And then the paging
unit translates to physical ram and then accesses without the program
ever knowing this happened. With our paging, we need a structure to
translate every possible virtual address into a physical address, we
can\'t do this directly as that would result in 4Bil entries, so this is
why we use a page and page table. We already have frames, so now our job
is to just trnaslate a virtual page to a physical frame.

In 32-bit mode, each adress is not treated as a single number any more
and is split into:

    31                22 21                12 11            0

    ┌──────────────────┬────────────────────┬────────────────┐
    │ Directory index  │ Table index        │ Offset         │
    └──────────────────┴────────────────────┴────────────────┘

            10 bits            10 bits          12 bits

First of all the bottom 12 bits are the offset. 2\^12 = 4096, so you can
imagine what this might represent. This tells where inside of the page
we are. The next 10 bits select an entry inside of the page table. Each
page table has 1024 entries as 2\^10 = 1024. The next 10 bits are the
page direcectory index, which point to a specific page table.

### Page directory/table structure

The page directory is a 1024 sized array which contains 32bits, each
entry is made up of a 20 bit page table address, and then 12 bits of
flags:

    31                    12 11          0
    +-----------------------+-------------+
    | Page table address    | Flags       |
    +-----------------------+-------------+

The page table address is physical.\
We only really need to concern ourself with the bottom 3 bits of the
flags:

-   Bit 0 is the present flag if it is 0, the page table does not exist,
    if it is 1, then it does
-   Bit 1 is the Writable flag. 0 = read only, 1 = writable
-   Bit 2 is the User flag, if 0, then the page table is kernel only, if
    1 then it is user accesible.

The page table has a similar structure:

    31                    12 11          0
    +-----------------------+-------------+
    | Physical frame addr   | Flags       |
    +-----------------------+-------------+

This also has the same flags but this time just to do with the pages
instead of the page tables.

### Enabling after creation

After the creation and fufillment of our page directories and tables, we
then need to tell the CPU that we want to enable paging. The first step
to do this is to fill the content of Control register 3 (CR3) This
contains where the current page directory is. So we must put the active
directory within there. When processes are switched, The directory for
the process will also be switched. (However for the current stage of our
OS we will end with there only being one page directory).

Then after this we must finally enable paging via changing CR0, which
controls whether major CPU features are active. This will automatically
activate paging and start translating adresses. So all we need to do is
Allocate a page directory, Allocate a page table, Load CR3 and then
enable paging. This seems easy, and this is because enabling paging is
the easiest part of this chapter. The actual complexity comes from
making our full virtual memory subsystem. We will cover this later in
the chapter, but for now let\'s make a simple implementation to get
paging up and running.

## The first implementation

For our first implementation we are just basically making the init
function and some extra functinons to help it, as well as making the
general structure for our VMM but the structure is rather simple.

    #ifndef VMM_H
    #define VMM_H

    #define PAGE_PRESENT (1 << 0)
    #define PAGE_WRITABLE (1 << 1)
    #define PAGE_USER (1 << 2)

    #include <stdint.h>

    typedef uint32_t page_directory_t[1024];
    typedef uint32_t page_table_t[1024];

    void init_vmm();

    extern void set_cr3(uintptr_t dir_ptr);
    extern void set_cr0();

    #endif

First we have our definitions, these are just flags for the page
directory and page table entries as we discussed before. Then there are
our data structures for our page table and page directory, it\'s simply
just a 1024 long aray of 32bit integers. Then we have our init_vmm
function which we will be writing. This requires helper functions for
setting our control registers.\
Now let\'s look at the implementation for this:

    #include "vmm.h"
    #include "../kernel/vga_text.h"
    #include "pmm.h"

    extern vga_text terminal;

    page_directory_t* kernel_directory;
    page_directory_t* current_directory;


    void init_vmm() {
        kernel_directory = alloc_frame();
        page_table_t* tbl_ptr = alloc_frame();

        memset(kernel_directory, 0, 4096);
        memset(tbl_ptr, 0, 4096);
        
        //gives every page up to the limit an entry in the table.
        for (uint32_t i = 0; i < 1024; i++) {
            (*tbl_ptr)[i] = (i * 0x1000) | PAGE_PRESENT | PAGE_WRITABLE;
        }

        (*kernel_directory)[0] = ((uintptr_t)tbl_ptr) | PAGE_PRESENT | PAGE_WRITABLE;    

        current_directory = kernel_directory;
        set_cr3((uintptr_t)kernel_directory);
        set_cr0();
    }

### The global variables

we have two global variables, the first is just the kernel directory,
this is because we need to save the kernel directory and have a special
one for it as it is the most critical program in an operating system.
The current directory for now is will just always have our kernel
directory loaded into it, but when we make processes and such we will
need to switch directories to access different process as every process
would typically have it\'s own page directory.

### The init code

To start our init code we allocate frames for our kernel directory and
the page table that we will be using for our kernel currently, then we
use the memset function that we made back in our interrupts code to set
the memory of the table and directory to 0, this is going to be a common
practice required for stuff like this, as memory isn\'t typically wiped,
it\'s just set as free.

After this we loop through and map each virtual address so that
`virtual address = physical address`. We set the flags for being Present
and writable, this is just so that when we turn on paging, we will still
be able to actually accesss memory, as if we didn\'t do this and we
tried to make a variable or do anything with memory, we would get a page
fault as each address would result in a page not being present.

We then make the first entry in the kernel directory that points to the
page table with the appropriate flags. the kernel directory is then
stored in the current directory and then we set CR3 and CR0 to turn on
paging.

### Assembly

``` language-x86asm
[BITS 32]

global set_cr3
global set_cr0

set_cr3:
    mov eax, [esp + 4]
    mov cr3, eax
    ret

set_cr0:
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    ret
```

just some simple code in order to load our desired values into the
respective registers, if we then link this code and run our program and
there are no faults, we should then have an operating system activated
with paging enabled. Perfect! That was simple, but now we should make
the functions and architecture that the other parts of our kernel can
use in order to map pages, unmap pages and detect page faults.

## Adapting the architecture

### The header file

    #ifndef VMM_H
    #define VMM_H

    #define PAGE_PRESENT (1 << 0)
    #define PAGE_WRITABLE (1 << 1)
    #define PAGE_USER (1 << 2)

    //page fault definitions
    #define PRESENT_FAULT (1 << 0)
    #define WRITE_FAULT (1 << 1)
    #define USER_FAULT (1 << 2)
    #define RESERVED_FAULT (1 << 3)
    #define INSTRUCTION_FETCH_FAULT (1 << 4)

    #include <stdint.h>
    #include "../kernel/interrupts.h"

    typedef uint32_t page_directory_t[1024];
    typedef uint32_t page_table_t[1024];

    void init_vmm();

    extern void set_cr3(uintptr_t dir_ptr);
    extern void set_cr0();
    extern  uint32_t get_cr2();

    //calc dir index, calc table index, find/create page table, insert page dir
    void map_page(
        uintptr_t virtual_address,
        uintptr_t physical_address,
        uint32_t flags
    );

    void unmap_page(uintptr_t virtual_address);
    uintptr_t get_physical_address(uintptr_t virtual_address);

    extern void flush_tlb(void);
    extern void flush_tlb_page(uintptr_t virtual_address);

    // Page table management

    page_table_t* create_page_table(uint32_t directory_index, uint32_t flags);

    // Page fault handling
    void page_fault_handler(registers_t* registers);



    #endif

In our header we just ahve added functions, we have one for recieving
CR2, which is used when we get to page faults. We then have our
functions for mapping pages, unmapping them, converting virtual
addresses to physical ones, creating page tables and then handling page
faults. These are all simple actions to conceptualise, but then we have
something we haven\'t talked about. The TLB is essentially a cache for
the most frequently used memory address translations, we won\'t need to
interface with it for now other than flushing when we delete pages and
tables.

### Implementation file

    #include "vmm.h"
    #include "../kernel/vga_text.h"
    #include "pmm.h"

    extern vga_text terminal;

    page_directory_t* kernel_directory;
    page_directory_t* current_directory;


    void init_vmm() {
        kernel_directory = alloc_frame();
        page_table_t* tbl_ptr = alloc_frame();

        memset(kernel_directory, 0, 4096);
        memset(tbl_ptr, 0, 4096);
        
        //gives every page up to the limit an entry in the table.
        for (uint32_t i = 0; i < 1024; i++) {
            (*tbl_ptr)[i] = (i * 0x1000) | PAGE_PRESENT | PAGE_WRITABLE;
        }

        (*kernel_directory)[0] = ((uintptr_t)tbl_ptr) | PAGE_PRESENT | PAGE_WRITABLE;    

        current_directory = kernel_directory;
        set_cr3((uintptr_t)kernel_directory);
        set_cr0();
    }

    page_table_t* create_page_table(uint32_t directory_index, uint32_t flags) {
        page_table_t* new_table = alloc_frame();
        memset(new_table, 0, 4096);
        (*current_directory)[directory_index] = (uintptr_t)new_table | flags;
        return new_table;
    }

    void map_page(uintptr_t virtual_address, uintptr_t physical_address, uint32_t flags) {
        uint16_t dir_index = (virtual_address >> 22);
        uint16_t table_index = (virtual_address >> 12 & 0x3FF); 

        uint32_t dir_entry = (*current_directory)[dir_index];
        page_table_t* selected_table;
        if (!(dir_entry & PAGE_PRESENT)) {
            selected_table = create_page_table(dir_index, flags);
        } else {
            selected_table = (page_table_t*)(dir_entry & 0xFFFFF000);
        }

        (*selected_table)[table_index] = physical_address | flags;
        flush_tlb_page(virtual_address);
    }

    void unmap_page(uintptr_t virtual_address) {
        uint16_t dir_index = (virtual_address >> 22);
        uint16_t table_index = (virtual_address >> 12 & 0x3FF); 

        uint32_t dir_entry = (*current_directory)[dir_index];
        page_table_t* selected_table = (page_table_t*)(dir_entry & 0xFFFFF000);
        (*selected_table)[table_index] &= ~(PAGE_PRESENT);
        flush_tlb_page(virtual_address);
    }

    uintptr_t get_physical_address(uintptr_t virtual_address) {
        uint16_t dir_index = (virtual_address >> 22);
        uint16_t table_index = (virtual_address >> 12 & 0x3FF); 

        uint32_t dir_entry = (*current_directory)[dir_index];
        page_table_t* selected_table = (page_table_t*)(dir_entry & 0xFFFFF000);
        return (*selected_table)[table_index];
    }

    void page_fault_handler(registers_t* registers) {

        uintptr_t address = get_cr2();
        uint32_t error_code = registers->error_code;

        vga_text_writeline(&terminal, "PAGE FAULT");

        vga_text_write(&terminal, "Address: ");
        vga_text_write_hex(&terminal, address);
        vga_text_writeline(&terminal, "");

        vga_text_write(&terminal, "Error code: ");
        vga_text_write_hex(&terminal, error_code);
        vga_text_writeline(&terminal, "");


        if (error_code & PRESENT_FAULT) {
            vga_text_writeline(&terminal, "Reason: Protection violation");
        }
        else {
            vga_text_writeline(&terminal, "Reason: Page not present");
        }


        if (error_code & WRITE_FAULT) {
            vga_text_writeline(&terminal, "Access: Write");
        }
        else {
            vga_text_writeline(&terminal, "Access: Read");
        }


        if (error_code & USER_FAULT) {
            vga_text_writeline(&terminal, "Mode: User");
        }
        else {
            vga_text_writeline(&terminal, "Mode: Kernel");
        }


        if (error_code & RESERVED_FAULT) {
            vga_text_writeline(&terminal, "Reserved bit violation");
        }


        if (error_code & INSTRUCTION_FETCH_FAULT) {
            vga_text_writeline(&terminal, "Instruction fetch fault");
        }


        vga_text_write(&terminal, "Instruction address: ");
        vga_text_write_hex(&terminal, registers->eip);
        vga_text_writeline(&terminal, "");


        // Stop execution
        while (1) {
            __asm__ volatile("cli; hlt");
        }
    }

Here the init function is mostly the same, let\'s take a look into all
the functions that i have made to suport the paging infrastructure.

### create_page_table

This function is pretty simple, we just allocate a physical frame, set
it\'s contents to 0, and then plug this into the current directory using
the given directory index and flags.

### map_page

This is one of the main functions to be used by the rest of our kernel.
We start by extracting the directory and table indexes by using a bit
shift. We next then check if the page table doesn\'t exist, if it
doesn\'t then we would need to create a new page table using the
previous function we just made, if there is a page table we just get
it\'s address by performing and on the entry with the bits that would
only have the address and not the flags.\
After we get our selected table we would then use the table index to set
the entry in the page table to the physical address. After this we then
flush the TLB which is like flushing the previous cache.

### unmap_page

This function is virtually the opposite of the previous, we just go to
the page (which is the same method in the previous functions) and then
set the present flag to 0. It\'s that simple.

### get_physical_address

This is mainly a function for if we ever want to debug in future, as we
may quickly need to convert a virtual address into a physical one. The
function works as previous but simply just returns the entry in the page
table (which would be the physical address)

### page_fault_handler

Page faults are caused by CPU exceptions, so we would want to call this
function from our ISR handler, as so:

    void isr_handler(registers_t* regs) {
        switch (regs->interrupt_number) {
            case 14:
                page_fault_handler(regs);
                break;
            default:
                vga_text_writeline(&terminal, exception_messages[regs->interrupt_number]);
                break;
        }

        for (;;);
    }

The quality of the page_fault_handler function helps finding errors in
our operating system in future, the more information we give about a
page fault, the better, as we can then use this to fix any issues we
have with memory access. In my function, CR2 contains the virtual
address that the fault happened at, this is a crucial piece of info that
we would definitely want to print.

Everything else comes from the bits of the error code given by our
registers_t type. So we just print eveyrything that we defined earlier
in our header, after this we also print out the latest instruction via
eip and halt.

### Assembly updated

And last but not least, the functions written in ASM, like flushing the
TLB and retrieving CR2:

``` language-x86asm
[BITS 32]

global set_cr3
global set_cr0
global flush_tlb
global flush_tlb_page
global get_cr2

set_cr3:
    mov eax, [esp + 4]
    mov cr3, eax
    ret

set_cr0:
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    ret

get_cr2:
    mov eax, cr2 
    ret

; flush entire tlb
; reload CR3 with itself
; cpu discards all cached virtual physcal translations

flush_tlb: 
    mov eax, cr3
    mov cr3, eax
    ret

flush_tlb_page:
    mov eax, [esp + 4]
    invlpg [eax]
    ret
```

flushing the whole TLB is pretty simple, as we just have to reload CR3
with itself and it all happens automatically, flushing a specific page
requires us to pass the virtual address to a special instruction that
will flush the tlb for the specific page. getting CR2 is the same as the
other control register functions etc.

And that\'s basically it for writing the VMM, pretty nice, all we have
left is heap allocation and then we are done with memory management
(however these files will probably be adapted as other parts are
created)

