#include "keyboard.h"
#include "interrupts.h"
#include "vga_text.h"

volatile bool shift_pressed = false;
volatile bool ctrl_pressed = false;
volatile bool alt_pressed = false;

extern vga_text terminal;

char keymap[128];
char shift_keymap[128];

void keyboard_init() {
    outb(PS2_DATA, ENABLE_SCANNING);
    keyboard_set_keymap();
    keyboard_set_shift_keymap();
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

void keyboard_set_shift_keymap(void)
{
    /* Numbers */
    shift_keymap[0x02] = '!';
    shift_keymap[0x03] = '@';
    shift_keymap[0x04] = '#';
    shift_keymap[0x05] = '$';
    shift_keymap[0x06] = '%';
    shift_keymap[0x07] = '^';
    shift_keymap[0x08] = '&';
    shift_keymap[0x09] = '*';
    shift_keymap[0x0A] = '(';
    shift_keymap[0x0B] = ')';

    /* Symbols */
    shift_keymap[0x0C] = '_';
    shift_keymap[0x0D] = '+';
    shift_keymap[0x1A] = '{';
    shift_keymap[0x1B] = '}';
    shift_keymap[0x27] = ':';
    shift_keymap[0x28] = '"';
    shift_keymap[0x29] = '~';
    shift_keymap[0x2B] = '|';
    shift_keymap[0x33] = '<';
    shift_keymap[0x34] = '>';
    shift_keymap[0x35] = '?';

    /* Top row */
    shift_keymap[0x10] = 'Q';
    shift_keymap[0x11] = 'W';
    shift_keymap[0x12] = 'E';
    shift_keymap[0x13] = 'R';
    shift_keymap[0x14] = 'T';
    shift_keymap[0x15] = 'Y';
    shift_keymap[0x16] = 'U';
    shift_keymap[0x17] = 'I';
    shift_keymap[0x18] = 'O';
    shift_keymap[0x19] = 'P';

    /* Home row */
    shift_keymap[0x1E] = 'A';
    shift_keymap[0x1F] = 'S';
    shift_keymap[0x20] = 'D';
    shift_keymap[0x21] = 'F';
    shift_keymap[0x22] = 'G';
    shift_keymap[0x23] = 'H';
    shift_keymap[0x24] = 'J';
    shift_keymap[0x25] = 'K';
    shift_keymap[0x26] = 'L';

    /* Bottom row */
    shift_keymap[0x2C] = 'Z';
    shift_keymap[0x2D] = 'X';
    shift_keymap[0x2E] = 'C';
    shift_keymap[0x2F] = 'V';
    shift_keymap[0x30] = 'B';
    shift_keymap[0x31] = 'N';
    shift_keymap[0x32] = 'M';

    /* Control characters */
    shift_keymap[0x01] = 27;
    shift_keymap[0x0E] = '\b';
    shift_keymap[0x0F] = '\t';
    shift_keymap[0x1C] = '\n';
    shift_keymap[0x39] = ' ';
}

void keyboard_extended_scancodes() {
    uint8_t scancode = inb(PS2_DATA);

    /* LGUI */
    if (scancode == 0x5B) {
        vga_text_writeline(&terminal, "LGUI PRESSED");
    }
}

bool keyboard_modifier_keys(uint8_t scancode) {
    /* shift */
    if (scancode == SHIFT_PRESS) {
        shift_pressed = true;     
        return true;
    } else if (scancode == (SHIFT_PRESS | KEY_RELEASED)) {
        shift_pressed = false;
        return true;
    }

    /* control */
    if (scancode == L_CTRL_PRESS) {
        ctrl_pressed = true;     
        return true;
    } else if (scancode == (L_CTRL_PRESS | KEY_RELEASED)) {
        ctrl_pressed = false;
        return true;
    }

    return false;
}

void keyboard_handler () {
    uint8_t scancode = inb(PS2_DATA);
    
    if (keyboard_modifier_keys(scancode)) {
        return;
    }
    if (scancode == EXTENDED_SCANCODE) {
        keyboard_extended_scancodes();
    }
    if (scancode & KEY_RELEASED) {
        return;
    }


    char c[2];
    c[0] = keymap[scancode];
    c[1] = '\0';
    switch (c[0]) {
        case '\n':
            vga_text_writeline(&terminal, "");
            break;
        case 27: 
            vga_text_clear(&terminal);
            break;
        case '\b':
            vga_text_backspace(&terminal);
            break;
        case '\t':
            vga_text_write(&terminal, "    ");
            break;
        default:
            if (shift_pressed) {
                c[0] = shift_keymap[scancode];
            }
            vga_text_write(&terminal, c);
            break;
    }
}



