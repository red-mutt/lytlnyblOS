# Creating the IDT

For this, we will need 3 files: `interrupts.asm`, `interrupts.c` and
`interrupts.h`

In our header file, the defined types in order to provide the building
blocks for our IDT are defined as such:

```c
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
```

Here the `__attribute__((packed))` attribute just makes it so the data in
memory is exactly as how we define it in the order we define it. The
structure we define here is going to be pretty similar to our GDT,
we just want a "descriptor," which in this case is an entry for our
interrupt, and then we need something like the GDTR data we had before
which describes our interrupt descriptor table, this being the `IDTR`.

We then need to define our functions here:

```c
void idt_init(void);

void idt_set_gate(
    uint8_t interrupt,
    uint32_t handler_address,
    uint16_t selector,
    uint8_t flags
);

void isr_handler(registers_t* regs);
```

`idt_init` and `idt_set_gate` are both pretty self-explanatory, but the
`isr_handler` is just made to handle what happens when an interrupt occurs.
Also, you may see the `regs` variable. This is defined using this type:

```c
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
```

This data structure is just so we can pass our registers when we go from
assembly to C in our code, which will happen when an interrupt is
called, speaking of assembly, we must also define external functions for
our assembly labels:

```c
extern void idt_load(idtr_t* idtr);

extern void isr0(void);
```

## Implementations of a basic IDT

To start with implementation of things, let's look at the
`interrupts.asm` file I have created:

```x86asm
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

The first line just allows Assembly to access the external function, and the
next two just allow C to access the Assembly labels `idt_load` is a small
piece of code that gets the data sent from C by accessing the address at
the stack pointer, and then it does the `lidt` instruction, we need to do
this because we cannot use the `lidt` instruction directly in C.

The `isr0` label is what we want the CPU to go to when interrupt 0 (DIV
by zero) is activated, it will then check our `IDT` for IDT entry 0, and
find the address of `isr0` via our entry. `dword` is used to push a 32-bit 
value onto the stack. Since this interrupt does not provide an error code, 
we push zero as a placeholder, we then use the `iret` as this is the instruction used
to return from an interrupt.

Now let's have a look at the C functions I've written.

```c
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
```

First, we instantiate our global variables and create a helper
function, which we might eventually move to a separate file containing other
helpful functions. For our init logic we just set the IDT to 0
to make sure that it's clear. We then set the `IDTR` values, and
then define our gates for every interrupt we want to
create. For the `idt_set_gate` function we arrange the data so that it
matches the IDT entry's required layout.

For now, our `isr_handler` simply prints that an exception has occurred and
then stalls. This is fine for now, as we are only testing whether 
everything else is working correctly.

Here is our new `main.c` file that I have written to test:

```c
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
```

The embedded Assembly code simply performs a division by zero. If we run
this, we should see that it tells us that an exception has occurred.
Now we should add all of our ISRs and define them within our
IDT.

