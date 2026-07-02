#include "keyboard.h"
#include "interrupts.h"
#include "vga_text.h"

volatile bool shift_pressed = false;
volatile bool ctrl_pressed = false;
volatile bool alt_pressed = false;

extern vga_text terminal;

char keymap[128];

void keyboard_init() {
    outb(PS2_DATA, ENABLE_SCANNING);
    keyboard_set_keymap();
}

void keyboard_set_keymap() {
    keymap[0x1E] = 'A';
}

void keyboard_handler () {
    uint8_t scancode = inb(PS2_DATA);
    vga_text_write_hex(&terminal, scancode);
}



