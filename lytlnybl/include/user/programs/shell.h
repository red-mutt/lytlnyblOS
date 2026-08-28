#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>

typedef struct {
  uint32_t last_status;
  uint32_t running;
  uint32_t interactive;
} Shell;

#endif
