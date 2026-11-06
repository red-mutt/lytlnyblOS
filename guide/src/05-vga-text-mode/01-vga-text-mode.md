# Part V : VGA Text Mode

In this chapter we are making a convenient way to output text to the screen
using the VGA text buffer we wrote to when we first reached C. This
task is a lot more like regular programming, so I will allow some creative
freedom. Here is the header file that I have created:

```c
#ifndef VGA_TEXT_H
#define VGA_TEXT_H

#include <stddef.h>
#include <stdint.h>

/// VGA CURSOR PORTS
#define CRTC_INDEX    0x3D4
#define CRTC_DATA     0x3D5
#define CRTC_HIGH_REG 0x0E
#define CRTC_LOW_REG  0x0F

typedef enum {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE,
    VGA_COLOR_GREEN,
    VGA_COLOR_CYAN,
    VGA_COLOR_RED,
    VGA_COLOR_MAGENTA,
    VGA_COLOR_BROWN,
    VGA_COLOR_LIGHT_GREY,
    VGA_COLOR_DARK_GREY,
    VGA_COLOR_LIGHT_BLUE,
    VGA_COLOR_LIGHT_GREEN,
    VGA_COLOR_LIGHT_CYAN,
    VGA_COLOR_LIGHT_RED,
    VGA_COLOR_LIGHT_MAGENTA,
    VGA_COLOR_LIGHT_BROWN,
    VGA_COLOR_WHITE
} vga_color;

typedef struct {
    size_t row;
    size_t column;

    size_t width;
    size_t height;

    uint8_t color;

    uint16_t* buffer;
} vga_text;

// init
void vga_text_init(vga_text* terminal);

//screen ops
//
void vga_text_clear(vga_text* terminal);

/* cursor ops */
void vga_text_set_cursor(
    vga_text* terminal
);

/* Color ops */
void vga_text_set_color(
    vga_text* terminal,
    vga_color f,
    vga_color b
);

/* char out */
void vga_text_putchar(
    vga_text* terminal,
    char c
);

/* string out */
void vga_text_write(
    vga_text* terminal,
    const char* string
);

void vga_text_writeline(
    vga_text* terminal,
    const char* string
);

/* num out */
void vga_text_write_dec(
    vga_text* terminal,
    uint32_t value 
);

void vga_text_write_hex(
    vga_text* terminal,
    uint32_t value
);

/* low level writing */

void vga_text_put_entry_at(
    vga_text* terminal,
    char character,
    uint8_t fcolor,
    uint8_t bcolor,
    size_t row,
    size_t column
);

#endif
```

All these definitions are pretty self-explanatory, other than the final
function. Which just writes a char without considering our main struct. For this
part of the guide I'll just give you all the implementations for my functions.
All this code is simple C with no OS-specific content. Try and use my
implementations as a rough guide and not as law. You can even make your own functions for writing
text entries instead of just characters as I did. 

```c
#include "vga_text.h"
#include "kernel/interrupts.h"

void vga_text_init(vga_text* terminal) {
    terminal->row = 0;
    terminal->column = 0;

    vga_text_set_color(terminal, VGA_COLOR_WHITE, VGA_COLOR_RED);

    terminal->width = 80;
    terminal->height = 25;

    terminal->buffer = (uint16_t*)0xB8000;
    vga_text_clear(terminal);
}

void vga_text_clear(vga_text* terminal) {
    uint8_t color = terminal->color;

    uint16_t blank = ((uint16_t)color << 8) | ' ';

    for (size_t row = 0; row < terminal->height; row++) {
        for (size_t col = 0; col < terminal->width; col++) {
            size_t index = row * terminal->width + col;
            terminal->buffer[index] = blank;
        }
    }
    
    terminal->row = 0;
    terminal->column = 0;

    vga_text_set_cursor(terminal);
}

void vga_text_set_color(vga_text* terminal, vga_color f, vga_color b) {
    terminal->color = ((uint8_t)b << 4) | (uint8_t)f; 
}

void vga_text_set_cursor(vga_text* terminal) {
    size_t cursor_pos = terminal->row * terminal->width + terminal->column;

    outb(CRTC_INDEX, CRTC_HIGH_REG);
    outb(CRTC_DATA, (uint8_t)(cursor_pos >> 0x8)); /// Write the high byte of the cursor position
    outb(CRTC_INDEX, CRTC_LOW_REG);
    outb(CRTC_DATA, (uint8_t)(cursor_pos & 0xFF)); /// Write the low byte of the cursor position
}

void vga_text_putchar(vga_text* terminal, char c) {
    uint8_t color = terminal->color;
    size_t index = terminal->row * terminal->width + terminal->column;
    uint16_t entry = ((uint16_t)color << 8) | (uint16_t)c;
    terminal->buffer[index] = entry;
}

void vga_text_write(vga_text* terminal, const char* string) {
    size_t pos = terminal->row * terminal->width + terminal->column;

    for (size_t i = 0; string[i] != '\0'; i++) {
        vga_text_putchar(terminal, string[i]);
        pos++;
        terminal->row = pos / terminal->width;
        terminal->column = pos % terminal->width;
        terminal->row = terminal->row % terminal->height;
    }

    vga_text_set_cursor(terminal);
}

void vga_text_writeline(vga_text* terminal, const char* string) {
    vga_text_write(terminal, string);
    terminal->column = 0;
    terminal->row++;
    terminal->row = terminal->row % terminal->height;

    vga_text_set_cursor(terminal);
}

void vga_text_write_dec(vga_text* terminal, uint32_t value) {
    char buffer[10];
    size_t digits = 0;
    if (value == 0) {
        vga_text_putchar(terminal, '0');
        return;
    }

    while (value > 0) {
        buffer[digits++] = '0' + (value % 10);
        value /= 10;
    }
    
    size_t index = 0;
    while (index < digits) {
        vga_text_putchar(terminal, buffer[--digits]);
        terminal->column++;
    }

    vga_text_set_cursor(terminal);
}

void vga_text_write_hex(vga_text* terminal, uint32_t value) {
    char buffer[8];
    size_t digits = 0;

    if (value == 0) {
        vga_text_putchar(terminal, '0');
        return;
    }

    while(value > 0) {
        uint32_t digit = value % 16;

        if (digit < 10) {
            buffer[digits++] = '0' + digit;
        } else {
            buffer[digits++] = 'A' + (digit - 10);
        }
        value /= 16;
    }

    size_t index = 0;
    while (index < digits) {
        vga_text_putchar(terminal, buffer[--digits]);
        terminal->column++;
    }

    vga_text_set_cursor(terminal);
}

void vga_text_put_entry_at(
    vga_text* terminal,
    char character,
    uint8_t fcolor,
    uint8_t bcolor,
    size_t row,
    size_t column
) {
    uint8_t color = ((uint8_t)bcolor << 4) | (uint8_t)fcolor; 
    size_t index = row * terminal->width + column;
    uint16_t entry = ((uint16_t)color << 8) | (uint16_t)character;
    terminal->buffer[index] = entry; 
}
```

`vga_text_putchar()` writes a character at the current cursor position, but it does not move
the cursor. The higher-level writing functions are responsible for moving the cursor.
This is the inverse to what you might expect for `putfoo()` functions, so I just thought I'd point
that out.

Notice that `vga_text_set_cursor()` also synchronizes our row and column with the physical VGA
hardware cursor. Writing characters to `0xB8000` only updates what is stored in video memory; the VGA
hardware renders the blinking underline cursor independently. To update its position on screen,
we calculate the flat index (`row * width + column`) and send its high and low bytes to the CRTC 
registers via `outb` using the VGA control ports (`0x3D4` and `0x3D5`).

The decimal and hexadecimal functions currently assume that there is enough
space remaining on the current row. Unlike `vga_text_write()`, they do not handle reaching
the end of a row, so just keep mind of that if you ever wish to use this near the 
end of a row.

And that's essentially this section complete.
