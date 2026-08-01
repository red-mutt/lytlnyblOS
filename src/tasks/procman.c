#include "procman.h"
#include "../memory/heap.h"

kprocess_t* process_head;
kprocess_t* current_process;
uint32_t next_pid;

void init_procman() {
    process_head = NULL;
    next_pid = INITIAL_PID;


    kprocess_t* kernel_process = kmalloc(sizeof(kprocess_t));
    kernel_process->pid = next_pid++;
    kernel_process->state = PROCESS_RUNNING;
    kernel_process->page_directory = get_current_directory();
    
    kprocess_registers_t kernel_regs;
    get_registers(&kernel_regs);
    kernel_process->regs = kernel_regs;
    kernel_process->next = NULL;

    kernel_process->kernel_stack = (void *)(0x9000 - KERNEL_STACK_SIZE);

    process_head = kernel_process;
    current_process = kernel_process;
}

kprocess_t* create_kprocess(void* task_address) {
    kprocess_t* new_process = kmalloc(sizeof(kprocess_t));
    new_process->pid = next_pid++;
    new_process->state = PROCESS_READY;
    new_process->kernel_stack = kmalloc(KERNEL_STACK_SIZE);
    new_process->next = NULL;
    
    new_process->regs.eax = 0;
    new_process->regs.ebx = 0;
    new_process->regs.ecx = 0;
    new_process->regs.edx = 0;
    new_process->regs.esi = 0;
    new_process->regs.edi = 0;
    new_process->regs.ebp = 0;

    new_process->regs.eip = (uintptr_t)task_address;
    new_process->regs.esp = (uint32_t)(new_process->kernel_stack) + KERNEL_STACK_SIZE;
    //set these to the gdt_code and gdt_data back in the first ASM file.
    new_process->regs.cs = 0x08;
    new_process->regs.ds = 0x10;
    //sensible eflags value
    new_process->regs.eflags = 0x202;

    new_process->page_directory = kernel_directory;

    kprocess_t* traversal_process = process_head;
    while (traversal_process) {
        if (!traversal_process->next) {
            traversal_process->next = new_process;
            break;
        }
        traversal_process = traversal_process->next;
    }
    return new_process;
}

void destroy_kprocess(kprocess_t *proc) {
    if (!proc || proc->state == PROCESS_RUNNING) return;

    kprocess_t* traversal_process = process_head;
    if (!traversal_process->next && traversal_process->pid == proc->pid) {
        process_head = NULL;
        kfree(proc->kernel_stack);
        kfree(proc);
        return;
    }

    while (traversal_process) {
        if (!traversal_process->next) {
            break;
        }

        if (traversal_process->next->pid == proc->pid) {
            traversal_process->next = traversal_process->next->next;
            break;
        } 
        traversal_process = traversal_process->next;
    }

    if (!traversal_process) {
        return;
    }

    kfree(proc->kernel_stack);
    kfree(proc);
}

kprocess_t* find_process_by_pid(uint32_t pid) {
    kprocess_t* traversal_process = process_head;
    while (traversal_process) {
        if (traversal_process->pid == pid) {
            return traversal_process;
        } 
        traversal_process = traversal_process->next;
    }
    return NULL;
}
