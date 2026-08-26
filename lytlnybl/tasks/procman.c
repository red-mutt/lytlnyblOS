#include "tasks/procman.h"
#include "memory/heap.h"
#include "memory/pmm.h"
#include "kernel/mappings.h"
#include "kernel/kernel_utils.h"

process_t* process_head;
process_t* current_process;
uint32_t next_pid;

void init_procman() {
    process_head = NULL;
    next_pid = INITIAL_PID;


    process_t* kernel_process = kmalloc(sizeof(process_t));
    kernel_process->pid = next_pid++;
    kernel_process->state = PROCESS_RUNNING;
    kernel_process->page_directory = current_directory;
    
    process_registers_t kernel_regs = {0};

    kernel_process->regs = kernel_regs;
    kernel_process->next = NULL;

    kernel_process->kstack = (void *)&kernel_stack_bottom;

    process_head = kernel_process;
    current_process = kernel_process;
}

process_t* create_process(void* task_address, process_type_t type) {
    process_t* new_process = kmalloc(sizeof(process_t));

    new_process->wake_tick = 0;
    new_process->type = type;
    new_process->pid = next_pid++;
    new_process->state = PROCESS_READY;
    new_process->kstack = kmalloc(KERNEL_STACK_SIZE);
    new_process->next = NULL;

    new_process->regs.eax = 0;
    new_process->regs.ebx = 0;
    new_process->regs.ecx = 0;
    new_process->regs.edx = 0;
    new_process->regs.esi = 0;
    new_process->regs.edi = 0;
    new_process->regs.ebp = 0;

    new_process->regs.eip = (uintptr_t)task_address;
    //esp defined based on type
    
    new_process->regs.eflags = 0x202;

    if (type == PROCESS_KERNEL) {
        create_kprocess(new_process);
    } else {
        create_uprocess(new_process);
    }

    process_t* traversal_process = process_head;
    while (traversal_process) {
        if (!traversal_process->next) {
            traversal_process->next = new_process;
            break;
        }
        traversal_process = traversal_process->next;
    }
    
    return new_process;
}

void create_kprocess(process_t* new_process) {
    new_process->regs.esp = (uint32_t)(new_process->kstack) + KERNEL_STACK_SIZE;
    new_process->regs.cs = 0x08;
    new_process->regs.ds = 0x10;
    new_process->regs.ss = 0x10;
    new_process->regs.eflags = 0x202;

    new_process->page_directory = kernel_directory;
}

void create_uprocess(process_t* new_process) {
    new_process->regs.esp = USER_STACK_TOP;
    
    new_process->regs.cs = 0x18 | 3;
    new_process->regs.ds = 0x20 | 3;
    new_process->regs.ss = 0x20 | 3;

    new_process->regs.eflags = 0x202;

    new_process->page_directory = (page_directory_t*)alloc_frame();
    memset((page_directory_t*)new_process->page_directory, 0, 4096);

    for (uint32_t i = 0; i < 1024; i++) {
        (*new_process->page_directory)[i] = (*kernel_directory)[i];
    }

    uintptr_t ustack_frame = (uintptr_t)alloc_frame();

    map_page(new_process->page_directory, 
            USER_STACK_TOP - 4096,
            ustack_frame,
            PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER
    );

    map_page(new_process->page_directory,
            USER_HEAP_START,
            (uintptr_t)alloc_frame(),
            PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER
    );
    /* if you ever try to create more mappings and get a general protection fault
     * it is likely that you have overwritten an existing kernel mapping, as we copy
     * the kernel mappings to the user process for when we use our interrupts, just
     * be careful of this */

    new_process->user_heap_end = USER_HEAP_START + 4096;
    new_process->heap_pages_allocated = 1;

    new_process->ustack = (void*)(USER_STACK_TOP - 4096);
}

void destroy_process(process_t* proc) {
    if (!proc || proc->state == PROCESS_RUNNING) return;

    // removing it from list
    process_t* traversal_process = process_head;
    while (traversal_process) {
        if (traversal_process->pid == process_head->pid && !traversal_process->next && traversal_process->pid == proc->pid) {
            process_head = traversal_process->next;
            break;
        }

        if (!traversal_process->next) {
            traversal_process = NULL;
            break;
        }

        if (traversal_process->next->pid == proc->pid) {
            traversal_process->next = traversal_process->next->next;
            break;
        } 
        traversal_process = traversal_process->next;
    }

    if (!traversal_process) return;

    kfree(proc->kstack);
    if (proc->type == PROCESS_USER) {
        unmap_page(proc->page_directory, USER_STACK_TOP - 4096);
        unmap_page(proc->page_directory, USER_CODE_BASE);
        unmap_page(proc->page_directory, USER_VGA);
        unmap_page(kernel_directory, (uintptr_t)(proc->page_directory));
    }
    kfree(proc);

}

process_t* find_process_by_pid(uint32_t pid) {
    process_t* traversal_process = process_head;
    while (traversal_process) {
        if (traversal_process->pid == pid) {
            return traversal_process;
        } 
        traversal_process = traversal_process->next;
    }
    return NULL;
}

tss_t tss;

void init_tss(void) {
    uintptr_t base = (uintptr_t)&tss;
    uint32_t limit = sizeof(tss_t) - 1;
    gdt_tss[0] = limit & 0xFF;
    gdt_tss[1] = (limit >> 8) & 0xFF;

    gdt_tss[2] = base & 0xFF;
    gdt_tss[3] = (base >> 8) & 0xFF;

    gdt_tss[4] = (base >> 16) & 0xFF;
    gdt_tss[5] = 0x89;

    gdt_tss[6] = (limit >> 16) & 0x0F;
    gdt_tss[7] = (limit >> 24) & 0xFF;

    memset(&tss, 0, sizeof(tss_t));

    tss.ss0 = 0x10;
    tss.esp0 = 0;

    tss.iomap_base = sizeof(tss_t);
    load_tss();
}
