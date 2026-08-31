# Part VIII : Keyboard Driver

## An overview of the hardware

In this part we are technically writing two drivers, one is for a PS/2
controller, this uses serial communication to talk to a PS/2 Keyboard.
Writing for this driver includes:

-   Reading bytes from the controller
-   Sending bytes
-   Enabling ports
-   Sending commands to the keyboard

All of these things being fairly low level.
The other device we are writing a driver for is the keyboard itself
which:

-   Recieves scan codes
-   Decodes scan codes
-   Tracks Shift/Ctrl/Alt
-   Converts scan codes into key events
-   Converts to ASCII

With this being more higher level after we complete the first task.

There are only 2 ports we need for the controller `0x60` for reading and
writing data, and `0x64` which gives the status bits when reading and
allows us to write commands to it when writing.

Reading from port 0x64 gives us:

    Bit 0
    Output buffer full

    Bit 1
    Input buffer full

    Bit 2
    System flag

    Bit 3
    Command/Data

    Bit 4
    Keyboard lock

    Bit 5
    Mouse output

    Bit 6
    Timeout

    Bit 7
    Parity Error

There are only really 2 that are important. These being output and input
buffer full, If output buffer full is set, then 0x60 contains data that
is waiting, if input buffer is set then the controller is busy and we
can\'t write yet. Another thing to note is that when the output buffer
is full, then IRQ1 happens.

When a key is pressed, the keyboard generates a scan code, the
controller recieves it and hten stores it whilst also raising IQR1, then
we can recieve it and decode. The keyboard does not send ASCII, it sends
scancodes. These are numbers that symbolise interactions with the
keyboard. For example `0x1E` symbolises A is pressed, and `0x9E`
symbolises release.

Some scancodes have multiple bytes (like right control) this is
symbolised by the 0xE0 prefix. The pause key is 8 bytes long, but we
will be ignoring this multimedia key of course.

For init we:

1.  Flush any pending data from the controller\'s output buffer
2.  Send 0xF4 to the keyboard to enable scanning (the bios may have
    already done this but just good practice
3.  Install the IRQ1 handler

Now let\'s get writing, starting with the header file which is all that
we need for our full implementation that we will make in this chapter:

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

    #include 
    #include 

    void keyboard_handler(void);
    void keyboard_init(void);
    void keyboard_set_keymap(void);
    void keyboard_set_shift_keymap(void);
    bool keyboard_modifier_keys(uint8_t scancode);
    void keyboard_extended_scancodes(void);

    #endif

With the KEY_RELEASED definition, this is OR\'d with a respective key\'s
scancode to signify that this key has been released, this will be
ignored other than for keys that we hold. We then have our data and
command ports and some commands, the SET_LED command is not needed. All
our functions are pretty self explanitory when we look at the
implementation.

Let\'s take a look at said implementation:

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

We then also need to call `keyboard_handler()` this in our irq handler
on case 1.

This is all we really need for our keyboard, we make an array that maps
each scancode to a respective key. Our init function is small, we just
enable scanning (which potentially may already have been done by the
bios) and set our keymap. in our handler we get the scancode and if it
contains the KEY_RELEASED definition we return. if not released we write
the key to the screen.

Also, this keymap assumes we are in set 1, this is the traditional PC AT
layout which is normally provided by BIOS, there are other sets but we
ignore them for now.

Really you could move on from this chapter with this basic
implementation and build up your own from this template with modifier
keys, handling extended keycodes, etc. (which is what the added
functions now included in the implementation from the header file
reference)

Here is my adapted implementation:

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

        vga_text_write_hex(&terminal, scancode);    
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

in our handler now we first call the modifier key function and this
returns true if a modifier key has been pressed and handles it
accordingly, if one is pressed we return as to not go through undefined
behaviour, we then check for extended scancodes and handle them. I have
added one, this being LGUI, i could have added more, but i haven\'t as
we really have no use for them, and our functions are pretty scalable
right now and we can add whatever we want.

With our shift key we also have a seperate keymap. You could make a
seperate implemetation if you wish to save memory. All of the keys that
aren\'t regular characters are pretty basic other than the backspace, i
made some new functions for this in our vga code. Let\'s take a look:

    void vga_text_backspace(vga_text* terminal) {
        if (terminal->row == 0 && terminal->column == 0) return;
        else if (terminal->column != 0) {
            terminal->column--;
        } else {
            terminal->row--;
            terminal->column = terminal->width - 1;
            while (vga_text_getchar(terminal) == ' ') {
                terminal->column--;
            }
        }
        vga_text_putchar(terminal, ' ');
    }

    char vga_text_getchar(vga_text* terminal) {
        size_t index = terminal->row * terminal->width + terminal->column;
        char c = (uint8_t)(terminal->buffer[index] & 0x00FF);
        return c;
    }

The backspace function moves the terminal cursor back once and deletes
the character if we are deep into a row and have text previous to us. If
we are at the start of a row, we then go back up to the row above at the
further right most column.

And then this is basically everything to do with keyboard managment,
some things obviously aren\'t implemented (like alt) but we don\'t
really have much use for these keys right about now, and we can easily
add them later, which isn\'t that much of a big deal anyway as we will
probably have to refactor alot of the code when we make a shell.

We will now move onto memory management.

