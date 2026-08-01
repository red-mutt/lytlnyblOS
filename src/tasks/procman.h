#ifndef PROCMAN_H
#define PROCMAN_H

#include <stdint.h>
#include <stddef.h>

#include "../kernel/interrupts.h"
#include "../memory/vmm.h"
#include "../memory/pmm.h"

#define INITIAL_PID 1
#define KERNEL_STACK_SIZE 0x4000

typedef enum {
    PROCESS_RUNNING = 0,
    PROCESS_READY = 1,
    PROCESS_BLOCKED = 2,
    PROCESS_SLEEPING = 3,
    PROCESS_TERMINATED = 4
} process_states_t;

typedef struct {
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

} kprocess_registers_t;

typedef struct next {
    uint32_t pid;
    kprocess_registers_t regs;
    process_states_t state;
    next* next;
    page_directory_t* page_directory;
    void* kernel_stack;
} kprocess_t; 

void init_procman();

process_t* create_kprocess(void* task_address);

void kdestroy_process(process_t* proc);

extern void get_registers(process_registers_t* regs);

process_t* find_process_by_pid(uint32_t pid);

#endif
