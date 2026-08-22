#include "chars.h"

int isalpha(int c) {
  return ((c >= 'a' && c <= 'z') ||
      (c >= 'A' && c <= 'Z'));
}

int isdigit(int c) {
  return (c >= '0' && c <= '9');
}

int isspace(int c) {
  return (c == ' ' ||
      c == '\t' ||
      c == '\n' ||
      c == '\v' ||
      c == '\f' ||
      c == '\r');
}

int islower(int c) {
  return (c >= 'a' && c <= 'z');
}

int isupper(int c) {
  return (c >= 'A' && c <= 'Z');
}

int tolower(int c) {
  if (isupper(c)) return c + ('a' - 'A');
  return c;
}

int toupper(int c) {
  if (islower(c)) return c - ('a' - 'A');
  return c;
}
