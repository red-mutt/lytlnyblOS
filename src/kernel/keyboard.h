#ifndef KEYBOARD_H
#define KEYBOARD_H

#define PS2_DATA 0x60
#define PS2_COMMAND 0x64

#define ENABLE_SCANNING 0xF4
#define DISABLE SCANNING 0xF5
#define SET_LED 0xED

#define KEY_RELEASED 0x80
#define EXTENDED_SCANCODE 0xE0

#define SHIFT_PRESS 0x2A
#define L_CTRL_PRESS 0x9C

#include <stdint.h>
#include <stdbool.h>

#include "../tasks/procman.h"

void keyboard_handler(void);
void keyboard_init(void);
void keyboard_set_keymap(void);
void keyboard_set_shift_keymap(void);
bool keyboard_modifier_keys(uint8_t scancode);
void keyboard_extended_scancodes(void);

#endif
