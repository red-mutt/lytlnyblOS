# Part VII : Timer Driver

## What is the timer

A driver is simply software that knows how to use a particular piece of
hardware. Technically, we have already written a driver (this being
the PIC driver)

In regard to the timer, we are interfacing with the Programmable
Interval Timer (PIT). All the PIT does is generate an interrupt after a
specified amount of time. This is good for a lot of reasons, we can use
this to keep track of how much time has passed, we can also do sleeping
for a specified amount of time, we will also use this if we want to
schedule tasks, and finally we can also implement timeouts and many more
things.

The timer is also simple to implement, as we just would really only
make 3 functions. One for initialization, one for handling the ticks,
and one for retrieving the current tick. Nice! The code I
present here will be similar to that short VGA section I provided
before.

To communicate we will be using the same `inb`/`outb` functions.
The PIT receives an input clock of around 1193182 Hz so we need to
calculate a divisor by doing `1193182 / desired_frequency`. The PIT then
counts down using this clock and generates an interrupt when the counter 
reaches zero.

Here is the header code for timer.h

```c
#ifndef TIMER_H
#define TIMER_H

/* PIT Ports */
#define PIT_CHANNEL0_DATA     0x40
#define PIT_CHANNEL1_DATA     0x41
#define PIT_CHANNEL2_DATA     0x42
#define PIT_COMMAND           0x43

/* PIT Input Clock */
#define PIT_BASE_FREQUENCY    1193182

/* Channel Selection */
#define PIT_CHANNEL0          0x00
#define PIT_CHANNEL1          0x40
#define PIT_CHANNEL2          0x80

/* Access Mode */
#define PIT_LATCH             0x00
#define PIT_ACCESS_LOBYTE     0x10
#define PIT_ACCESS_HIBYTE     0x20
#define PIT_ACCESS_LOHIBYTE   0x30

/* Operating Modes */
#define PIT_MODE0             0x00
#define PIT_MODE1             0x02
#define PIT_MODE2             0x04
#define PIT_MODE3             0x06
#define PIT_MODE4             0x08
#define PIT_MODE5             0x0A

/* Counting Mode */
#define PIT_BINARY            0x00
#define PIT_BCD               0x01

#include <stdint.h>

void timer_init(uint32_t frequency);

void timer_handler(void);

uint32_t timer_get_ticks(void);

#endif
```

Like with ICW, i have definitions for most commands and addresses for
the PIT, the mode we are using is mode 3 which is the square wave mode.
The PIT's data ports are 8 bits wide, but our divisor is 16 bits. This
is why we have different access modes. We use `PIT_ACCESS_LOHIBYTE`,
which tells the PIT that we will send the divisor in two parts: 
the low byte first and the high byte second. The latch mode allows
us to capture the current count so that we can safely read it.
Without latching the value, the counter could change between
reading the low byte and reading the high byte.

We don't need the latch for our timer yet, but it can be useful later
if we want to read the current count from the PIT.

Here's what the modes do for the PIT

-   **Mode 0** is a one-show timer that generates its output after 
    a certain amount of time. This is not what we need
-   **Mode 1** is the same as 0 but only starts or restarts when there
    is an external trigger
-   **Mode 2** is a rate generator that repeatedly generates pulses at a regular rate.
-   **Mode 3** is similar to Mode 2, but generates a square wave. This
    is the mode we will use for our regular timer interrupts.
-   **Mode 4** is a software triggered strobe
-   **Mode 5** is the same as 4 but from an external hardware trigger

Another thing to note is that PIT has 3 channels, which can be used
for different timing purposes. We are using channel 0 for our system timer.

Now let's look at all of our implementations:

```c
#include "timer.h"
#include "interrupts.h"
#include "vga_text.h"

volatile uint32_t ticks = 0;

extern vga_text terminal;

void timer_init(uint32_t frequency) {
    uint16_t divisor = PIT_BASE_FREQUENCY / frequency;

    /* tell pit how we send the divisor value and the mode*/
    outb(PIT_COMMAND, PIT_ACCESS_LOHIBYTE | PIT_MODE3 | PIT_CHANNEL0 | PIT_BINARY);
    io_wait();

    /* write low and high bytes respectively */
    outb(PIT_CHANNEL0_DATA, divisor & 0xFF);
    io_wait();
    outb(PIT_CHANNEL0_DATA, divisor >> 8);
    io_wait();
}

void timer_handler(void) {
    ticks++;
    if ((ticks % 100) == 0) {
        vga_text_writeline(&terminal, " 1 second ");
    }
}

uint32_t timer_get_ticks() {
    return ticks;
}
```

In our init we calculate the divisor and configure channel 0 to use mode 3,
binary counting, and the low-byte/high-byte access mode. We 
then send the low byte of the divisor followed by the high byte. 
BCD means binary-coded decimal, where each decimal digit is represented 
using binary. We are using binary counting rather than BCD counting.

We then write the 16-bit divisor to channel 0 as two 8-bit values. We send the
low byte first and then the high byte.
That's our initialization set up. The timer handler's code is just for debugging.
Because we configured the PIT to generate 100 interrupts per second, every
100 ticks should be approximately one second. This lets us check that our 100Hz
timer is working correctly.
Later on we may need to consider setting ticks to a 64-bit value.
A 32-bit OS can still use 64-bit integers, although
operations on them may require more instructions. For now, a 32-bit
value is enough for our initial testing and further development with the 
operating system. We can change it to 64 bits later so that the tick counter can 
run for much longer before it wraps around.

We can then call this from our IRQ manager:

```c
void irq_handler(registers_t* regs) {
    switch (regs->interrupt_number - 32) {
        case 0:
            timer_handler();
            break;
        case 1:
            break;
        case 2:
            break;
    }
    pic_send_eoi(regs->interrupt_number - 32);
}
```

Remember that our hardware IRQs start at interrupt vector 32 because
we remapped the PIC earlier. Therefore, IRQ0 arrives as interrupt number 32. 
Subtracting 32 gives us the original IRQ number, which is 0 for the timer.

For now, we only handle IRQ0. We will add the other hardware devises to this
switch statement as we write their drivers

Call `timer_init(100)` from main after calling `idt_init()` to
initialize the timer and then test. You should see 1 second show up
every second. If you get any errors with putting `timer_init(100)` after
`idt_init()` this could be because the PIT expects to be configured *before
interrupts are enabled*. Otherwise, the CPU can start receiving timer
interrupts before you've configured the PIT with desired frequency.
To fix this, you may want to move `asm volatile("sti");` in main rather than 
`idt_init()`.

The timer is finished. That was simple. Now let's move onto writing the
keyboard driver.

