#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

char** tokenize_line(const char* line, size_t *count_out);
bool execute_command(char** words, size_t word_count); 



#endif
