#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdarg.h>
#include "syscalls.h"

void printf(char* format, ...);
int putchar(int c);
int puts(char* str);

#endif
