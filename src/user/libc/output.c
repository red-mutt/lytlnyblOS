#include "output.h"

int putchar(int c) {
   
  return 0;
}

int puts(char* str) {

  return 0;
}

void printf(char* format, ...) {
  char *traverse;
  unsigned int i;
  char *s;

  va_list arg;
  va_start(arg, format);

  for(traverse = format; *traverse != '\0'; traverse++) {
    while (*traverse == '%') {
      putchar(*traverse);
      traverse++;
    }
  }
}
