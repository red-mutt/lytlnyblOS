#include "kernel_utils.h"
 
void* memset(void* dest, uint8_t val, size_t len) {
  uint8_t* d = (uint8_t*)dest;
 
  for (size_t i = 0; i < len; i++)
    d[i] = val;
 
  return dest;
}
 
void* memcpy(void* dest, const void* src, size_t len) {
  uint8_t* d = (uint8_t*)dest;
  const uint8_t* s = (const uint8_t*)src;
 
  for (size_t i = 0; i < len; i++)
    d[i] = s[i];
 
  return dest;
}
 
int32_t strcmp(const char* a, const char* b) {
  size_t i = 0;
 
  while (a[i] != '\0' && b[i] != '\0' && a[i] == b[i])
    i++;
 
  return (uint8_t)a[i] - (uint8_t)b[i];
}
 
char* strncpy(char* dest, const char* src, size_t n) {
  size_t i = 0;
 
  for (; i < n && src[i] != '\0'; i++)
    dest[i] = src[i];
 
  for (; i < n; i++)
    dest[i] = '\0';
 
  return dest;
}

