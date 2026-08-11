#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "procman.h"
#include "../kernel/interrupts.h"
#include "context.h"

#define DEFAULT_TIME_SLICE 10

process_t* get_next_process();

void schedule(registers_t* regs);

#endif
