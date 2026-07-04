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

void keyboard_set_keymap(void)
{
    /* Numbers */
    keymap[0x02] = '1';
    keymap[0x03] = '2';
    keymap[0x04] = '3';
    keymap[0x05] = '4';
    keymap[0x06] = '5';
    keymap[0x07] = '6';
    keymap[0x08] = '7';
    keymap[0x09] = '8';
    keymap[0x0A] = '9';
    keymap[0x0B] = '0';

    /* Symbols */
    keymap[0x0C] = '-';
    keymap[0x0D] = '=';
    keymap[0x1A] = '[';
    keymap[0x1B] = ']';
    keymap[0x27] = ';';
    keymap[0x28] = '\'';
    keymap[0x29] = '`';
    keymap[0x2B] = '\\';
    keymap[0x33] = ',';
    keymap[0x34] = '.';
    keymap[0x35] = '/';

    /* Top row */
    keymap[0x10] = 'q';
    keymap[0x11] = 'w';
    keymap[0x12] = 'e';
    keymap[0x13] = 'r';
    keymap[0x14] = 't';
    keymap[0x15] = 'y';
    keymap[0x16] = 'u';
    keymap[0x17] = 'i';
    keymap[0x18] = 'o';
    keymap[0x19] = 'p';

    /* Home row */
    keymap[0x1E] = 'a';
    keymap[0x1F] = 's';
    keymap[0x20] = 'd';
    keymap[0x21] = 'f';
    keymap[0x22] = 'g';
    keymap[0x23] = 'h';
    keymap[0x24] = 'j';
    keymap[0x25] = 'k';
    keymap[0x26] = 'l';

    /* Bottom row */
    keymap[0x2C] = 'z';
    keymap[0x2D] = 'x';
    keymap[0x2E] = 'c';
    keymap[0x2F] = 'v';
    keymap[0x30] = 'b';
    keymap[0x31] = 'n';
    keymap[0x32] = 'm';

    /* Control characters */
    keymap[0x01] = 27;      /* Escape */
    keymap[0x0E] = '\b';    /* Backspace */
    keymap[0x0F] = '\t';    /* Tab */
    keymap[0x1C] = '\n';    /* Enter */
    keymap[0x39] = ' ';
}

void keyboard_handler () {
    uint8_t scancode = inb(PS2_DATA);
    //vga_text_write_hex(&terminal, scancode);

    if (scancode & KEY_RELEASED) {
        return;
    }

    char c[2];

    c[0] = keymap[scancode];
    c[1] = '\0';
    vga_text_write(&terminal, c);
}



