#ifndef STRING_H
#define STRING_H

#include <stddef.h>

size_t strlen(const char *str);
int strcmp(const char *str1, const char *str2);
int strncmp(const char *str1, const char *str2, size_t n);
char* strcpy(char* dest, const char *src);
char* strcat(char* dest, const char *src);
char* strchr(const char *str, int c);
char* strstr(const char *text, const char *search);


#endif
