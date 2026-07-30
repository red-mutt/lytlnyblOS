#ifndef PROCMAN_H
#define PROCMAN_H

#include <stdint.h>
#include <stddef.h>

#include "../kernel/interrupts.h"
#include "../memory/vmm.h"

#define INITIAL_PID 1;
#define KERNEL_STACK_SIZE 0x4000;

typedef enum {
    PROCESS_RUNNING = 0,
    PROCESS_READY = 1,
    PROCESS_BLOCKED = 2,
    PROCESS_SLEEPING = 3,
    PROCESS_TERMINATED = 4
} process_states_t;

typedef struct {
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

} process_registers_t;

typedef struct process_t {
    uint32_t pid;
    process_registers_t* regs;
    process_states_t state;
    process_t* next;
    page_directory_t* page_directory;
    void* kernel_stack;
} process_t; 

void init_procman();

process_t* create_process();

void destroy_process(process_t* proc);

extern void get_registers(process_registers_t* regs);

#endif
