## Creating the IDT

For this, we will need 3 files: interrupts.asm, interrupts.c and
interrupts.h

In our header file, the defined types in order to provide the building
blocks for our IDT are defined as such:

    typedef struct {
        uint16_t offset_low;
        uint16_t selector;
        uint8_t reserved;
        uint8_t flags;
        uint16_t offset_high;
    } __attribute__((packed)) idt_entry_t;

    typedef struct {
        uint16_t limit;
        uint32_t base;
    } __attribute__((packed)) idtr_t;

here the `__attribute__((packed)) idtr_t;` just makes it so the data in
memory is exactly as how we define it in the order we define it. The
structure of how we define this is going to pretty similar to our GDT,
we just want a \"descriptor\", which in this case is an entry for our
interrupt and then we need something like the GDTR data we had before
which describes or interrupt descriptor table, this being the IDTR.

We then need to define our functions here:

    void idt_init(void);

    void idt_set_gate(
        uint8_t interrupt,
        uint32_t handler_address,
        uint16_t selector,
        uint8_t flags
    );

    void isr_handler(registers_t* regs);

`idt_init` and `idt_set_gate` are both pretty self explanitory, but the
isr_hander is just made to handle what happens when an interrupt occurs.
Also you may see the regs variable, this is defined from this type:

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

    } __attribute__((packed)) registers_t;

This data structure is just so we can pass our registers when we go from
assembly to C in our code, which will happen when an interrupt is
called, speaking of assembly, we must also define external functions for
our assembly labels:

    extern void idt_load(idtr_t* idtr);

    extern void isr0(void);

## Implementations of a basic IDT

To start with implementation of things, let\'s look at the
interrupts.asm file i have created:

``` language-x86asm
[BITS 32]

extern isr_handler

global idt_load
global isr0

idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret

isr0:
    push dword 0
    call isr_handler
    add esp, 4
    iret
```

the first line just allows asm to acccess the external function, and the
next two just allow C to access the asm labels idt_load is just a small
piece of code that gets the data sent from C by accessing the adress at
the stack pointer, and then it does the lidt instruction, we need to do
this as we would not have access to the lidt instruction in C

The `isr0` label is what we want the CPU to go to when interrupt 0 (div
by zero) is activated, it will then check our idt for IDT entry 0, and
then will find the address of isr0 via our entry. dword is used to push
our registers to the stack, as this is just divide by 0, we will not be
needing them and hence just push zero, we then use the iret as this is a
special command especially used for returning from interrupts

Now let\'s take a look at the C functions i\'ve written.

    #include "interrupts.h"
    #include "vga_text.h"

    extern vga_text terminal;

    idt_entry_t idt[256];
    idtr_t idtr;

    static void memset(void* ptr, uint8_t val, uint32_t size) {
        uint8_t* p = ptr;

        for (uint32_t i = 0; i < size; i++) {
            p[i] = val;
        }
    }

    void idt_set_gate(
        uint8_t interrupt,
        uint32_t handler_address,
        uint16_t selector,
        uint8_t flags
    ) {
        idt[interrupt].offset_low = handler_address & 0xFFFF;

        idt[interrupt].selector = selector;

        idt[interrupt].reserved = 0;

        idt[interrupt].flags = flags;

        idt[interrupt].offset_high = (handler_address >> 16) & 0xFFFF;
    }

    void isr_handler(registers_t* regs) {
        vga_text_writeline(&terminal, "Exception occured");

        for(;;)
        {
        }
    }

    void idt_init(void) {
        memset(idt, 0, sizeof(idt));

        idtr.limit = sizeof(idt) - 1;

        idtr.base = (uint32_t)idt;

        idt_set_gate(
            0,
            (uint32_t)isr0,
            0x08,
            0x8E
        );

        idt_load(&idtr);
    }

First we instantiate our global vars and also make a pretty nice helper
funcition, which honestly we might move to a seperate file with alot of
helpful functions later on. For our init logic we just set the idt to 0.
(making sure that it is clear), and then we set the idtr values, and
then we would then define out set gates for every gate we want to
create. For the `idt_set_gate` function we just arrange the data how it
should be with the idt entry\'s special arrangement.

For nouw our ist_handler just prints that an exception has occured and
then stalls, and that is fine for now as we are just testing that
everything else is working.

Here is our new mainfile that i have written to test:

    #include "vga_text.h"
    #include "interrupts.h"

    vga_text terminal;

    void kernel_main(void)
    {
        volatile char* vga = (volatile char*)0xB8000;
        
        //signal that we have reached C
        vga[0] = 'C';
        vga[1] = 0x02;

        vga_text_init(&terminal);
        vga_text_writeline(&terminal, "Welcome to the lytlnybl kernel in real mode");
        vga_text_writeline(&terminal, "Interrupts coming soon...");

        idt_init();
        
        asm volatile (
            "xor %%edx, %%edx\n"
            "mov $10, %%eax\n"
            "div %%edx"
            :
            :
            : "eax", "edx"
        );

        for (;;);
    }

The embedded asm code simply just does a division by zero, if we run
this we should see that it should tell us that an exeption has occured,
fantastic! Now we should add all of our isrs and define them within our
idt

