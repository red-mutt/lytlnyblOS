#include "string.h"

size_t strlen(const char *str) {
  size_t len = 0;

  while (str[len] != '\0') len++;

  return len;
}

int strcmp(const char *str1, const char *str2) {
  while (*str1 && *str1 == *str2) {
    str1++;
    str2++;
  }
  return *str1 - *str2;
}
