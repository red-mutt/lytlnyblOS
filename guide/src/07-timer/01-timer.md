# Part VII : Timer Driver

## What is the timer

A driver is simply software that knows how to use a particular piece of
hardware. Technically, we have already written a driver (this being
the PIC driver).

In regard to the timer, we are interfacing with the Programmable
Interval Timer (PIT). All the PIT does is generate an interrupt after a
specified amount of time. This is good for a lot of reasons, we can use
this to keep track of how much time has passed, we can also do sleeping
for a specified amount of time, we will also use this if we want to
schedule tasks, and finally we can also implement timeouts and many more
things.

The timer is also vary simple to implement, as we just would really only
make 3 functions. One for initialization, one for handling the ticks,
and one for retrieving the current tick. Nice! The code I
present here will be similar to that sort VGA section I provided
before.

To communicate we will be using the same `inb`/`outb` functions.
The pit always receives an internal clock of around 1193182 Hz so we
need to calculate a divisor by doing 1193182/desiredfreq. It will then
count from our divisor to 0 every CPU tick.

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

#include 

void timer_init(uint32_t frequency);

void timer_handler(void);

uint64_t timer_get_ticks(void);

#endif
```

Like with ICW, i have definitions for most commands and addresses for
the PIT, the mode we are using is mode 3 which is the square wave mode
the PIT can only recieve 8 bytes at a time, whcih is why we have those
accessing modes, with the latch being where we pause operation in order
to read the count within the PIT, this is because the we can read the
low byte and then the high byte can change by the time we read that, so
we need to pause.

Here\'s what the modes do for the PIT

-   **Mode 0** is a mode one shot timer that executes after a certain
    ammount of time. This is not what we need
-   **Mode 1** is the same as 0 but only starts or restarts when there
    is an external trigger
-   **Mode 2** is a rate generator which is usually used for baud rate
    etc
-   **Mode 3** is the same as 2 but is a square wave where 50% of time
    is spent on and 50% of time is off
-   **Mode 4** is a software triggered strobe
-   **Mode 5** is the same as 4 but from an external hardware trigger

Another thing to note is that the PIT has 3 channels which can all be
used for seperate timers

Now let\'s look at all of our implementations:

```c
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
        vga_text_writeline(&terminal, " 1 second ");
    }
}

uint64_t timer_get_ticks() {
    return ticks;
}
```

In our init we set the divisor and send a command to change the low byte
then the high byte, then set the PIT_MODE to 3, on channel 0 and make
sure that the value in the PIT is treated as binary. BCD means binary
coded decimal where every decimal digit is a seperate bin value (this is
used for some old harware in the 1970s and 1980s).

we then write the divisor 8bytes at a type to the channel 0\'s data. and
that\'s our initialization set up. The timer handler\'s code is just a
debug which will output when 1 second has passed so we can ensure that
100hz has been set correctly.\
Later on we may need to consider setting the ticks to a 64bit value. it
originally was, but if it was we wouldn\'t be able to perform a modulus
operator on it when testing, so you may want to change it to 64 bits
after ensuring that it works. (and then make some special code for
performing on 64bit values.

We can then call this from our iqr manager:

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

call `timer_init(100)` from main after calling `idt_init()` to
initialise the timer and then test. you should see 1 second show up
every second.

The timer is finished. That was simple. Now let\'s move onto writing the
keyboard driver.

