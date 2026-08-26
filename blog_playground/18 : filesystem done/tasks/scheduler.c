#include "tasks/scheduler.h"
#include "tasks/context.h"

volatile uint32_t scheduler_tick_count = 0;
uint32_t time_slice = DEFAULT_TIME_SLICE;

process_t* get_next_process() {
    process_t* traversal_process = current_process;
    do {
        if (traversal_process->state == PROCESS_READY) {
            return traversal_process;
        }

        if (!traversal_process->next) {
            traversal_process = process_head;
        } else {
            traversal_process = traversal_process->next;
        }
    } while (!(traversal_process->state == PROCESS_RUNNING));

    return current_process;
}

void schedule(registers_t* regs) {
    scheduler_tick_count++;
    if (scheduler_tick_count >= time_slice) { 
        scheduler_tick_count = 0;
        process_t* next_process = get_next_process();
        if (next_process != current_process) {
            context_switch(current_process, next_process, regs);
        }
    }
    return;
}
