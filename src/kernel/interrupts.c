#include "interrupts.h"
#include "vga_text.h"
#include "timer.h"
#include "keyboard.h"
#include "../memory/vmm.h"

extern vga_text terminal;

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


idt_entry_t idt[256];
idtr_t idtr;

void memset(void* ptr, uint8_t val, uint32_t size) {
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

void irq_handler(registers_t* regs) {
    switch (regs->interrupt_number - 32) {
        case 0:
            timer_handler(regs);
            break;
        case 1:
            keyboard_handler();
            break;
        case 2:
            break;
    }
    pic_send_eoi(regs->interrupt_number - 32);
}

void idt_init(void) {
    memset(idt, 0, sizeof(idt));

    idtr.limit = sizeof(idt) - 1;

    idtr.base = (uint32_t)idt;

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


    idt_load(&idtr);
    pic_remap(0x20, 0x28);
    asm volatile("sti");
}

void pic_send_eoi(uint8_t irq){
	if(irq >= 8)
		outb(PIC2_COMMAND, PIC_EOI);
	
	outb(PIC1_COMMAND,PIC_EOI);
}

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
