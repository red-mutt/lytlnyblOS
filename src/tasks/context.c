#include "context.h"

void save_context(kprocess_t* process, registers_t* regs) {
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

void load_context(kprocess_t* process, registers_t* regs) {
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

void context_switch(kprocess_t* old_process, kprocess_t* new_process, registers_t* regs) {
    current_process = new_process;
    save_context(old_process, regs);
    load_context(new_process, regs);
}
