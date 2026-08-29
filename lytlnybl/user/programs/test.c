#include <stdint.h>
#include "user/libc/output.h"
#include "user/libc/memory.h"

#include "user/libc/syscalls.h"



void _start(void)
{
    char input[5];

    char text[10] = "hellohello";
    write(1, (void*)input, 5);
    putchar('a');
    putchar('b');
    putchar('\n');
    printf("i have a number it is: %d \n", 314);


    printf("malloc: ");

    char *ptr = malloc(10);
    ptr[0] = 'h';
    ptr[1] = 'i';
    ptr[2] = '\0';

    puts(ptr);

    free(ptr);

    printf("\ncalloc: ");

    char *ptr2 = calloc(5, 1);
    ptr2[0] = 'a';
    ptr2[1] = 'b';
    ptr2[2] = 'c';
    ptr2[3] = '\0';

    puts(ptr2);

    free(ptr2);

    printf("\nrealloc: ");

    char *ptr3 = malloc(5);

    ptr3[0] = 'h';
    ptr3[1] = 'e';
    ptr3[2] = 'l';
    ptr3[3] = 'l';
    ptr3[4] = '\0';

    ptr3 = realloc(ptr3, 10);

    ptr3[4] = 'o';
    ptr3[5] = 'o';
    ptr3[6] = 'o';

    puts(ptr3);
    printf("\n");

    free(ptr3);

    exit();
}
