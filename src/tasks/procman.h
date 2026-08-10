#ifndef PROCMAN_H
#define PROCMAN_H

#include <stdint.h>
#include <stddef.h>

#include "../kernel/interrupts.h"
#include "../memory/vmm.h"
#include "../memory/pmm.h"

#define INITIAL_PID 1
#define KERNEL_STACK_SIZE 0x4000
#define USER_STACK_TOP 0xBFFFF000
#define NEW_PAGE_DIR_VIRT 0xD0000000

extern uint8_t kernel_stack_bottom;

typedef enum {
    PROCESS_RUNNING = 0,
    PROCESS_READY = 1,
    PROCESS_BLOCKED = 2,
    PROCESS_SLEEPING = 3,
    PROCESS_TERMINATED = 4
} process_states_t;

typedef struct {
    uint32_t ss;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t ds;

    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

} __attribute__((packed)) process_registers_t;

typedef enum {
    PROCESS_KERNEL,
    PROCESS_USER
} process_type_t;

typedef struct process {
    uint32_t pid;
    
    process_registers_t regs;

    process_states_t state;
    process_type_t type;

    struct process* next;

    page_directory_t* page_directory;

    void* kstack;
    void* ustack;
} __attribute__((packed)) process_t; 


void init_procman();

process_t* create_kprocess(void* task_address);

void destroy_kprocess(process_t* proc);

process_t* find_process_by_pid(uint32_t pid);

extern process_t* current_process;
extern process_t* process_head;

typedef struct {
    uint32_t prev_tss;

    uint32_t esp0;
    uint32_t ss0;

    uint32_t esp1;
    uint32_t ss1;

    uint32_t esp2;
    uint32_t ss2;

    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;

    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;

    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;

    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;

    uint32_t ldt;

    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed)) tss_t;

extern uint8_t tss[];
extern uint8_t tss_end[];
extern tss_t* tss_ptr;

void init_tss(void);
void set_kernel_stack(uintptr_t stack);
extern void load_tss(void);

#endif
