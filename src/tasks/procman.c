#include "procman.h"
#include "../memory/heap.h"

process_t* process_head;
process_t* current_process;
uint32_t next_pid;

void init_procman() {
    process_head = NULL;
    next_pid = INITIAL_PID;


    process_t* kernel_process = kmalloc(sizeof(process_t));
    kernel_process->pid = next_pid++;
    kernel_process->state = PROCESS_RUNNING;
    kernel_process->page_directory = get_current_directory();
    
    process_registers_t* kernel_regs;
    get_registers(kernel_regs);
    kernel_process->regs = kernel_regs;
    kernel_process->kernel_stack = (void*)(kernel_process->regs->esp);

    process_head = kernel_process;
    current_process = kernel_process;
}

process_t* create_process() {
    process_t* new_process = kmalloc(sizeof(process_t));
    new_process->pid = next_pid++;
    new_process->state = PROCESS_READY;
    new_process->kernel_stack = kmalloc(KERNEL_STACK_SIZE);
    
    new_process->regs->eax = 0;
    new_process->regs->ebx = 0;
    new_process->regs->ecx = 0;
    new_process->regs->edx = 0;
    new_process->regs->esi = 0;
    new_process->regs->edi = 0;
    new_process->regs->ebp = 0;

    
}
