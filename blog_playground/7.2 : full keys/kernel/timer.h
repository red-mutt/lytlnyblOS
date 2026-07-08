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

uint64_t timer_get_ticks(void);

#endif
