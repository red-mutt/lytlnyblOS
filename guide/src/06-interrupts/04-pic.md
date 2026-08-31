## Programmable Interrupt Controller

Every interrupt we have now comes from the CPU itself, these are self
contained exceptions. But what about when some other piece of hardware
needs to send an interrupt? Well this is where a piece of hardware
called the Programmable Interrupt Controller (PIC) comes in.

The PIC handles interrupt requests (IRQs) which are basically hardware
interrupts. And ther are 16 of them. For example IQR0 handles the timer
and IQR1 handles the keyboard and so on and so forth. Annoyingly due to
old IBM design, the IQR\'s are mapped overlapping with some of our ISRs.
For example, right now, if we invoke ISR0, it will invoke ISR8 as they
share the same interrupt vector. So we must remap them to 32 and so on.

In our architecture the 16 IQRs are split up between 2 PICs, a master
and a slave which each handle it\'s own set of 8 They are physically
connected The slave\'s PIC output is connected to the master\'s IRQ2
input

In order to communicate with the PIC(s), we must first get their IO base
addresses, here are some definitions for that:

    //PORT DEFINITIONS

    #define PIC1        0x20        /* IO base address for master PIC */
    #define PIC2        0xA0        /* IO base address for slave PIC */
    #define PIC1_COMMAND    PIC1
    #define PIC1_DATA   (PIC1+1)
    #define PIC2_COMMAND    PIC2
    #define PIC2_DATA   (PIC2+1)

We communicate to the PIC (WHEN INITIALIZING) using Initialization
Command Words (ICWs) There are 4: 1 handles the start of initialization,
2 handles where the interrupt vector begins, 3 handles how the master
and slave are connected, and 4 handles the mode. Here are some ICW codes
that i have defined:

    //ICW DEFINITIONS

    #define ICW1_ICW4 0x01
    #define ICW1_SINGLE 0x02
    #define ICW1_INTERVAL4 0x04
    #define ICW1_LEVEL 0x08
    #define ICW1_INIT 0x10

    #define ICW4_8086 0x01
    #define ICW4_AUTO 0x02
    #define ICW4_BUF_SLAVE 0x08
    #define ICW4_BUF_MASTER 0x0C
    #define ICW4_SFNM 0x10

These constants represent all expressions that can be made with the ICW
commands. We will use `ICW1_ICW4` which tells us that ICW4 will be
following after ICW1. We will also use `ICW_INIT` (obviously) and
`ICW_8086` as this is our mode.

Let\'s take a look at our function definitions for Setting up the PIC,
setting up IQRs and handling IQRs:

    void irq_handler(registers_t* regs);

    void pic_remap(int offset1, int offset2);

    extern void outb(uint16_t port, uint8_t value);
    extern uint8_t inb(uint16_t port);
    extern void io_wait(void);

    void pic_send_eoi(uint8_t irq);

    extern void irq0(void);
    extern void irq1(void);
    extern void irq2(void);
    extern void irq3(void);
    extern void irq4(void);
    extern void irq5(void);
    extern void irq6(void);
    extern void irq7(void);
    extern void irq8(void);
    extern void irq9(void);
    extern void irq10(void);
    extern void irq11(void);
    extern void irq12(void);
    extern void irq13(void);
    extern void irq14(void);
    extern void irq15(void);

Here we have 3 helper functions `outb` simply sends a byte to I/O, `inb`
recieves a byte from I/O, `io_wait` is a rather primitive version of a
time out that writes to an unused port in order to give I/O enough time
to finish processing the previous command (real timouts will come when
we make our timer driver, but what we have here isn\'t bad in any way)

now that we know the helper functions, let\'s take a look at the
pic_remap function first, as this should be the first step in our logic
of handling the PIC. The function looks like this:

    void pic_remap(int offset1, int offset2) {
        //save state of enabled IRQs
        uint8_t a1 = inb(PIC1_DATA);
        uint8_t a2 = inb(PIC2_DATA);

        /* Enter initialization mode. */
        outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
        io_wait();

        outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
        io_wait();

        /* set up first interrupt vector used by master and slave */

        outb(PIC1_DATA, offset1);
        io_wait();

        outb(PIC2_DATA, offset2);
        io_wait();

        /* Connect master and slave through IRQ2 line */
        outb(PIC1_DATA, 4);
        io_wait();

        outb(PIC2_DATA, 2);
        io_wait();

        /* select 8086/x86 interrupt mode */
        outb(PIC1_DATA, ICW4_8086);
        io_wait();

        outb(PIC2_DATA, ICW4_8086);
        io_wait();

        /* restore interrupt masks */
        outb(PIC1_DATA, a1);
        outb(PIC2_DATA, a2);

    }

You can see in this code that when we use ICWs, we need to send the
commands both to the master and slave PICs. The first thing we do is use
the OR operator in order to say that we are initializing and performing
an ICW4 after this. For ICW2 we send the offset address that is intended
in the IDT for both master and slave which are seperate. We then connect
the master and the slave by setting bit 2 of the data given to the
master and by sending a value of 2 to the slave (which indicates connect
through output). We then select x86 mode and then restore the masks.

Now let\'s take a look at our helper functions:

``` language-x86asm
outb:
    mov dx, [esp + 4]
    mov al, [esp + 8]
    out dx, al
    ret
inb:
    mov dx, [esp + 4]
    in al, dx
    movzx eax, al
    ret
io_wait:
    mov al, 0
    out 0x80, al
    ret
```

Simple, remember to add global statements so they are visible to C.

That\'s PIC remapping all set up now we can look at setting it up with
the IDT, this is very similar to what we did before with the ISRs, First
let\'s set them up when initializing the IDT.

        idt_set_gate(32, (uint32_t)irq0,  0x08, 0x8E);
        idt_set_gate(33, (uint32_t)irq1,  0x08, 0x8E);
        idt_set_gate(34, (uint32_t)irq2,  0x08, 0x8E);
        idt_set_gate(35, (uint32_t)irq3,  0x08, 0x8E);
        idt_set_gate(36, (uint32_t)irq4,  0x08, 0x8E);
        idt_set_gate(37, (uint32_t)irq5,  0x08, 0x8E);
        idt_set_gate(38, (uint32_t)irq6,  0x08, 0x8E);
        idt_set_gate(39, (uint32_t)irq7,  0x08, 0x8E);
        idt_set_gate(40, (uint32_t)irq8,  0x08, 0x8E);
        idt_set_gate(41, (uint32_t)irq9,  0x08, 0x8E);
        idt_set_gate(42, (uint32_t)irq10, 0x08, 0x8E);
        idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);
        idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);
        idt_set_gate(45, (uint32_t)irq13, 0x08, 0x8E);
        idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);
        idt_set_gate(47, (uint32_t)irq15, 0x08, 0x8E);

Then we can write a macro that handles them:

``` x86asm
global irq0
global irq1
global irq2
global irq3
global irq4
global irq5
global irq6
global irq7
global irq8
global irq9
global irq10
global irq11
global irq12
global irq13
global irq14
global irq15

%macro IRQ 2
irq%1:
    push dword 0
    push dword %2
    jmp irq_common_stub
%endmacro

IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47
```

Pushing only 0 for the error code. Then we have our stub which is the
same other than the function we call:

    irq_common_stub:
        pusha

        mov ax, ds
        push eax

        push esp
        call irq_handler
        add esp, 4

        pop eax

        popa

        add esp, 8
        iret

Now let\'s take a look at the basic handler, here it is:

    void irq_handler(registers_t* regs) {
        vga_text_writeline(&terminal, "IRQ");
        pic_send_eoi(regs->interrupt_number - 32);
    }

The text is there for later testing. However the there is another thing
we do. EOI stands for end of instruction and it\'s a code we send to the
PIC to indicate the end of the instruction, the PIC then cleans up for
us rather than us manually having to return or something. Here\'s our
functio for it, as well as a definitions we need:

    void irq_handler(registers_t* regs) {
        vga_text_writeline(&terminal, "IRQ");
        pic_send_eoi(regs->interrupt_number - 32);
    }
    #define PIC_EOI     0x20        /* End-of-interrupt command code */

Now the final thing to do before we test is to add this:

        pic_remap(0x20, 0x28);
        asm volatile("sti");

To the end of our idt init file. The latter being an instruction that
activates interrupts. Now when we run our code and resize the data
loaded byt he bootloader we can see that our text will print to the
terminal over and over. This is done by the timer. Which we will be
writing drivers for as our next step.

