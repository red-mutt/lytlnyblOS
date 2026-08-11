#include "context.h"
#include "../kernel/vga_text.h"
extern vga_text* terminal;

void save_context(process_t* process, registers_t* regs) {
    process->regs.ss = regs->ss;
    process->regs.eip = regs->eip;
    process->regs.cs = regs->cs;
    process->regs.eflags = regs->eflags;
    process->regs.ds = regs->ds;

    process->regs.edi = regs->edi;
    process->regs.esi = regs->esi;
    process->regs.ebp = regs->ebp;
    process->regs.esp = regs->esp;
    process->regs.ebx = regs->ebx;
    process->regs.edx = regs->edx;
    process->regs.ecx = regs->ecx;
    process->regs.eax = regs->eax;
}

void load_context(process_t* process, registers_t* regs) {
    regs->ss = process->regs.ss;
    regs->eip = process->regs.eip;
    regs->cs = process->regs.cs;
    regs->eflags = process->regs.eflags;
    regs->ds = process->regs.ds;

    regs->edi = process->regs.edi;
    regs->esi = process->regs.esi;
    regs->ebp = process->regs.ebp;
    regs->esp = process->regs.esp;
    regs->ebx = process->regs.ebx;
    regs->edx = process->regs.edx;
    regs->ecx = process->regs.ecx;
    regs->eax = process->regs.eax;
}

void context_switch(process_t* old_process, process_t* new_process, registers_t* regs) {
    current_process = new_process;
    old_process->state = PROCESS_READY;
    new_process->state = PROCESS_RUNNING;
    save_context(old_process, regs);

    if (new_process->type == PROCESS_USER) {  
        tss.esp0 = (uintptr_t)(new_process->kstack) + KERNEL_STACK_SIZE;
        set_cr3((uintptr_t)new_process->page_directory);
    } else if (new_process->type == PROCESS_KERNEL) {
        set_cr3((uintptr_t)kernel_directory);
    }

    load_context(new_process, regs);
}
