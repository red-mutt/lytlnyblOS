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

int strncmp(const char *str1, const char *str2, size_t n) {
  size_t i = 0;

  while (i < n) {
    char c1 = str1[i];
    char c2 = str2[i];

    if (c1 != c2) return c1 - c2;

    if (c1 == '\0') return 0;

    i++;
  }

  return 0;
}

char* strcpy(char *dest, const char *src) {
  while ((*dest++ = *src++) != '\0');
  return dest;
}

char* strcat(char* dest, const char* src) {
  while (*dest)dest++;
  while ((*dest++ = *src++) != '\0');
  return dest;
}

char* strchr(const char *str, int c) {
  while (*str) {
    if (*str == c) return (char *)str;
    str++;
  }

  if (c == '\0') return (char *)str;

  return NULL;
}

char* strstr(const char *text, const char *search) {
  size_t search_length;
  size_t i;
  size_t j;

  search_length = strlen(search);

  for (i = 0; text[i] != '\0'; i++) {
    for (j = 0; j < search_length; j++) {
      if (text[i + j] == '\0') break;

      if (text[i + j] != search[j]) break;
    } 

    if (j == search_length) return (char*)&text[i];
  }

  return NULL;
}




