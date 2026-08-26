#include "interrupts.h"
#include "vga_text.h"
#include "timer.h"
#include "keyboard.h"
#include "../memory/vmm.h"
#include "mappings.h"

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

    if ((regs->cs & 3) == 3) {
        current_process->state = PROCESS_TERMINATED;
        context_switch(current_process, get_next_process(), regs);
        return;
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

            fd = regs->ebx;
            buffer = regs->ecx;
            count = regs->edx;

            if (fd != 0) { 
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
    
    //software interrupts
    idt_set_gate(0x80, (uint32_t)syscall_entry, 0x08, 0xEE);


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
