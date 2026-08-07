#ifndef CONTEXT_H
#define CONTEXT_H

#include "../tasks/procman.h"
#include <stdint.h>


void context_switch(kprocess_t* old_process, 
        kprocess_t* new_process, 
        registers_t* regs);

void save_context(kprocess_t* process, registers_t* regs);

void load_context(kprocess_t* process, registers_t* regs);

#endif
