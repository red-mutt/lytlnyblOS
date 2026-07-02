#ifndef KEYBOARD_H
#define KEYBOARD_H

#define PS2_DATA 0x60
#define PS2_COMMAND 0x64

#define ENABLE_SCANNING 0xF4
#define DISABLE SCANNING 0xF5
#define SET_LED 0xED

#include <stdint.h>
#include <stdbool.h>

void keyboard_handler(void);
void keyboard_init(void);
void keyboard_set_keymap(void);

#endif
