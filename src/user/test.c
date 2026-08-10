void _start(void)
{
    volatile unsigned short* vga = (unsigned short*)0xB8000;

    vga[0] = 'U' | (0x07 << 8);

    while (1) {
    }
}
