#include <stdint.h>
void _start(void)
{
    volatile uint32_t *bad_address = (uint32_t *)0xDEADBEEF;

    *bad_address = 1234;

    volatile unsigned short* vga = (unsigned short*)0xB8000;

    vga[0] = 'U' | (0x07 << 8);

    while (1) {
    }
}
