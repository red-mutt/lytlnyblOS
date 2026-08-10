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
    
    process_registers_t kernel_regs = {0};

    kernel_process->regs = kernel_regs;
    kernel_process->next = NULL;

    kernel_process->kstack = (void *)&kernel_stack_bottom;

    process_head = kernel_process;
    current_process = kernel_process;
}

process_t* create_kprocess(void* task_address) {
    process_t* new_process = kmalloc(sizeof(process_t));
    new_process->type = PROCESS_KERNEL;
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
    new_process->regs.esp = (uint32_t)(new_process->kstack) + KERNEL_STACK_SIZE;
    //set these to the gdt_code and gdt_data back in the first ASM file.
    new_process->regs.cs = 0x08;
    new_process->regs.ds = 0x10;
    new_process->regs.ss = 0x10;
    //sensible eflags value
    new_process->regs.eflags = 0x202;

    new_process->page_directory = kernel_directory;

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

process_t* create_uprocess(void* task_address) {
    process_t* new_process = kmalloc(sizeof(process_t));
    new_process->type = PROCESS_USER;
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
    new_process->regs.esp = USER_STACK_TOP;
    //set these to the gdt_code and gdt_data back in the first ASM file.
    new_process->regs.cs = 0x18 | 3;
    new_process->regs.ds = 0x20 | 3;
    new_process->regs.ss = 0x20 | 3;
    //sensible eflags value
    new_process->regs.eflags = 0x202;

    new_process->page_directory = (page_directory_t*)alloc_frame();
    //need to map new page directory to a page in the kernel directory so that we can use a 
    //virtual address to edit data
    map_page(NEW_PAGE_DIR_VIRT, 
            (uintptr_t)new_process->page_directory, 
            PAGE_PRESENT | PAGE_WRITABLE);
    memset((page_directory_t*)NEW_PAGE_DIR_VIRT, 0, 4096);

    for (uint32_t i = 0; i < 1024; i++) {
        (*((page_directory_t*)NEW_PAGE_DIR_VIRT))[i] = (*kernel_directory)[i];
    }

    set_cr3((uintptr_t)new_process->page_directory);
    uintptr_t ustack_frame = (uintptr_t)alloc_frame();
    map_page(USER_STACK_TOP - 4096, ustack_frame, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
    new_process->ustack = (void*)(USER_STACK_TOP - 4096);

    set_cr3((uintptr_t)kernel_directory);

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

void destroy_kprocess(process_t *proc) {
    if (!proc || proc->state == PROCESS_RUNNING) return;

    process_t* traversal_process = process_head;
    if (!traversal_process->next && traversal_process->pid == proc->pid) {
        process_head = traversal_process->next;
        kfree(proc->kstack);
        kfree(proc);
        return;
    }

    while (traversal_process) {
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

    if (!traversal_process) {
        return;
    }

    kfree(proc->kstack);
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

tss_t *tss_ptr = (tss_t*)tss;
uint32_t tss_size = sizeof(*tss_ptr);

void init_tss(void) {
    memset(&tss, 0, sizeof(*tss_ptr));

    tss_ptr->ss0 = 0x10;
    tss_ptr->esp0 = 0;

    tss_ptr->iomap_base = sizeof(*tss_ptr);
    load_tss();
}
