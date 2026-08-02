#include "timer.h"
#include "interrupts.h"
#include "vga_text.h"

volatile uint32_t ticks = 0;
static uint32_t freq;

extern vga_text terminal;

void timer_init(uint32_t frequency) {
    freq = frequency;
    uint16_t divisor = 1193182 / frequency;

    /* tell pit how we send the divisor value and the mode*/
    outb(PIT_COMMAND, PIT_ACCESS_LOHIBYTE | PIT_MODE3 | PIT_CHANNEL0 | PIT_BINARY);
    io_wait();

    /* write low and high bytes respectively */
    outb(PIT_CHANNEL0_DATA, divisor & 0xFF);
    io_wait();
    outb(PIT_CHANNEL0_DATA, divisor >> 8);
    io_wait();
}

void timer_handler() {
    ticks++;
    if ((ticks % 100) == 0) {
        //vga_text_writeline(&terminal, " 1 second ");
    }
}

uint64_t timer_get_ticks() {
    return ticks;
}
