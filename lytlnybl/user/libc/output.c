#include "user/libc/output.h"

int putchar(int c) {
  return write(1, &c, 1) ? 1 : -1;    
}

int puts(char* str) {
  while(*str != '\0') {
    if (!putchar(*str)) return -1;
    str++;
  }
  return 1;
}

void print_num(int num) {
  if (num == 0) {
    putchar('0');
    return;
  }

  if (num < 0) {
    putchar('-');
    num = -num;
  }

  char buffer[10];
  int i = 0;

  while (num > 0) {
    buffer[i++] = (num % 10) + '0';
    num /= 10;
  }

  while (i--) {
    putchar(buffer[i]);
  }

}

void printf(char* format, ...) {
  char *traverse;
  unsigned int i;
  char *s;

  va_list args;
  va_start(args, format);

  while (*format) {
    if (*format == '%') {
      format++;
      if (*format == 'c') {
        char c = va_arg(args, int);
        putchar(c);
      } else if (*format == 's') {
        char *str = va_arg(args, char*);
        puts(str);
      } else if (*format == 'd') {
        int num = va_arg(args, int);
        print_num(num);
      } else if (*format == '%') {
        putchar('%');
      } else {
        //unknown format, so print the raw
        putchar('%');
        putchar(*format);
      }
    } else {
      putchar(*format);
    }
    format++;
  }
  va_end(args);
}
