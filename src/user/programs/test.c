#include <stdint.h>

#define SYSCALL_EXIT 0x01

#include "../libc/syscalls.h"



void _start(void)
{
    /*
    volatile uint32_t *bad_address = (uint32_t *)0xDEADBEEF;

    *bad_address = 1234;
    */
    

    volatile unsigned short* vga = (unsigned short*)0x00B00000;

    vga[0] = get_pid() + '0' | (0x07 << 8);

    //char input[5];
    //read(0, (void*)input, 5);

    sbrk(4096);

    char text[10] = "hellohello";
    //write(1, (void*)input, 5);

    
    vga[1] = 'P' | (0x07 << 8);


    exit();
}
