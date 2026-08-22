#ifndef CONTEXT_H
#define CONTEXT_H

#include "../tasks/procman.h"
#include <stdint.h>


void context_switch(process_t* old_process, 
        process_t* new_process, 
        registers_t* regs);

void save_context(process_t* process, registers_t* regs);

void load_context(process_t* process, registers_t* regs);

extern volatile bool return_to_user;

#endif
