# libc

I\'m sure if you\'ve gotten up to this point; you know what a C standard
library is. That is what we are implementing here. We actually have 2
different choices we can take here, we can either:

-   **A)** Implement our own version of libc.
-   **B)** Take an existing version of libc and port it to our operating
    system.

For the latter, this would require us to have 17 syscalls (According to
POSIX standards) that we would then use to weld together the libc and
the existing kernel. These syscalls being:

1.  \_exit
2.  close\_
3.  envion\_
4.  execve\_
5.  fork\_
6.  fstat\_
7.  getpid\_
8.  isatty\_
9.  kill\_
10. link\_
11. lseek\_
12. open\_
13. read\_
14. sbrk
15. stat\_
16. times\_
17. unlink\_
18. wait\_
19. write\_

However, folling the trend of us making all of our own stuff up until
now (for example, the bootloader) and the fact that we currently do not
even have a file system or a full set of syscalls, some of which depend
on filesystems. I will not be instructing you how to port a libc; we
will be writing our own (very minimal) version of libc. You can consider
porting a libc later if you so wish, or you could continue developing
the libc as you go along with creating this operating system, adding new
stuff with each added functionality to the operating system.

Please be warned however, writing your own libc takes a very long
ammount of time, using an existing one would allow you to focus much
more on the development of the OS, rather than the libc. Another issue
with using your own libc is that it gives us the ability to port our
existing software, therefore with our own it\'s very possible for this
porting of software to not work. (If you like doom, this is the main
thing required to get doom running on your OS :0)

## The functions we are making

Because we want a very minimal version of libc, i can list to you
everything that we will be making, here:

    --memory:
    malloc
    free
    calloc
    realloc
    memcpy
    memmove
    memset
    memcmp

    --string
    strlen
    strcmp
    strncmp
    strcpy
    strcat
    strchr
    strrchr
    strstr

    --output
    putchar
    puts
    printf
    (fputs??)

    --character stuffs
    isalpha
    isdigit
    isalnum
    isspace
    islower
    isupper
    tolower
    toupper

You may notice that in the previous section, we did not write any
syscalls that interface with the heap allocator we wrote for our kernel,
this is because the kernel\'s heap and a user process\'s heap are kept
completely seperate, we will be writing a completely seperate heap
allocator here. This makes the memory section the hardest section in our
libc, but it\'s nothing we haven\'t done before, so we should be fine!

Basically, writing our libc isn\'t really going to be too hard, but with
all these functions and the requirement of a new memory allocator, it is
going to take a bit of a long time. This chapter will be structured by
me going one by one and getting you aquainted with the information
required to make a libc and then i will show you my implementation of a
libc after.

## Memory allocation

The memory allocator is the largest task here. So we will do that first.

