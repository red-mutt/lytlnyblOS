# C standard library

I'm sure if you've got up to this point; you know what the C standard
library is (also known as libc). That is what we are writing here. We actually have 2
different choices we can take here, we can either:

-   **A)** Implement our own version of libc.
-   **B)** Take an existing version of libc and port it to our operating
    system.

For the latter, this would require us to have 17 syscalls (According to
POSIX standards) that we would then use to weld together the libc and
the existing kernel. These syscalls being:

```
1.  _exit
2.  close_
3.  envion_
4.  execve_
5.  fork_
6.  fstat_
7.  getpid_
8.  isatty_
9.  kill_
10. link_
11. lseek_
12. open_
13. read_
14. sbrk
15. stat_
16. times_
17. unlink_
18. wait_
19. write_
```

Following the trend of us making all of our own stuff up until
now (for example, the bootloader) and the fact that we currently do not
even have a file system or a full set of syscalls, some of which depend
on file systems. I will not be instructing you how to port a libc; we
will be writing our own (minimal) version of libc. You can consider
porting a libc later if you so wish, or you could continue developing
the libc as you go along with creating this operating system, adding new
stuff with each added functionality to the operating system.

Please be warned, writing your own libc takes a long
amount of time, using an existing one would allow you to focus much
more on the development of the OS, rather than the libc. Another issue
with using your own libc is that it gives us the ability to port our
existing software, with our own implementation it's possible for this
porting of software to not work. (If you like doom, this is the main
thing required to get doom running on your OS :0)

## The functions we are making

Because we want a minimal version of libc, I can list to you
everything that we will be making, here:

```
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
```

You may notice that in the previous section, we did not write any
syscalls that interface with the heap allocator we wrote for our kernel,
this is because the kernel's heap and a user process's heap are kept
separate, we will be writing a different heap
allocator here. This makes the memory section the hardest section in our
libc, but it's nothing we haven't done before, so we should be fine!

Basically, writing our libc isn't really going to be too hard, but with
all these functions and the requirement of a new memory allocator, it's
going to take a bit of a long time. This chapter will be structured by
me going one by one and getting you acquainted with the information
required to make a libc, and then I will show you my implementation of a
libc after.

### Memory allocation and manipulation

The memory allocator is the largest task here. We will do that first. We can
just take most of the kernel heap code to build the user one. I've already
explained how the heap works in that section, we don't really need to cover it
that again.

There will be some quirks with heap allocation in user space though, this is because
we cannot directly allocate pages, we must use the `SBRK` syscall instead which will
increase a heap by our desired size.

On top of the basic heap functions that are: `malloc`, `free`, `find_free_block`, `expand_heap`
`split_block`, `merge_blocks`. We must also make extra functionality: `memmove`, `memset`,
`memcmp`, `memcpy`, `calloc` and `realloc`. The first 4 are just privative functions for basic
memory manipulation, the last two: `calloc` and `realloc` are simply wrappers for `malloc` that allocate
functions in particular ways. 

#### New Allocation

`realloc` stands for "re-allocate" 
and frees memory that has already been allocated, and then reallocates 
it using `malloc` in a place with a new size. `calloc` stands for "contiguous allocation,"
it allocates memory for an array of elements, initializes all bytes in the allocate
storage to zero, and returns the pointer just like `malloc`.

#### New primitives

`memcmp` compares memory it iterates through two pointers for a specified count 
and returns the compare status. The status is 0 if they are the same, 1 if the de-referenced value at the
pointer one is bigger than the one at pointer two. -1 is returned if the inverse is true.

`memset` sets memory of count n to a specified value, that's it.

`memcpy` copies memory from one address to the next for a given count, 
however this function doesn't have protection for if the location we copy to
overwrites the source that we copy from. `memmove` does the exact same thing but
does have this protection. It does this by checking if `destination > source`, if this condition
is true, we copy the data backwards so that source isn't overwritten. This works because the address of 
the destination is greater than source, so if the data in the source bleeds into the destination, the data of 
source that gets overwritten is copied first before being overwritten. If you don't understand you'll see when we
get to the implementation.

### String Manipulation

This is a set of what I think to be the most used functions in libc for string manipulation.

We have `strlen`, which iterates a count until we reach the null terminator, when we do, we just return
the count which is now the length.\
`strcmp` which iterates through the two strings until we reach the end or a character that is different what 
returns is the ASCII code difference in the two characters that are different or 
just 0 if they are the same.\
`strncmp` which compares like the last one, but does it for a specified number of characters.\
`strcpy` which copies a string from a source to a destination.\
`strcat` which concatenates one string to the end of another.\
`strchr` this searches for a character within a string and returns the pointer to the character if it's found,
`NULL` gets returned if there is nothing.\
`strstr` searches for if there is a string within another string and returns the address like the last.

### Output

All output functions will be based on the `write` syscall that we made 
in the previous chapter. The most basic of output being `putchar` where
a single character writes to `stdout`. The `puts` function `puts` gets built
on top of this, this will output each character in a string passed until the null 
terminator gets found.

On top of `puts` and `putchar`. The function that you are most familiar with will then be made.
This being `printf`. This is where we will have to make an algorithm that takes in
a format, scans it, and uses a variable number of arguments to replace the format specifiers
with content passed to the function.

### Character Manipulation

Most of the functions for this part will just be a single line,
we just have function for checking what characters are. You can tell
what these do by their names easily.

## Implementations

### Memory 

```c
#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../../kernel/mappings.h"
#include "syscalls.h"

typedef struct heap_header{
  size_t size;
  bool free;
  struct heap_header* next;
} heap_header_t;

void* malloc(size_t bytes);
void free(void* ptr);
int memcmp(void* ptr1, void* ptr2, size_t n);
void* memcpy(void* dest, const void* src, size_t n);
void* memmove(void* dest, const void* src, size_t bytes);
void* memset(void* ptr, uint8_t c, size_t n);
void* calloc(size_t n, size_t size);
void* realloc(void* ptr, size_t new_size);

#endif
```

Our header here actually has a level of protection functions for things like 
expansion and merging aren't seen here as we don't want them to be accessible in regular user 
space code. Everything gets defined in our full implementation file here:

```c
#include "user/libc/memory.h"

heap_header_t* heap_start_head;
uintptr_t heap_end;

void init_heap() {
  heap_end = USER_HEAP_START + 4096;
  heap_start_head = (void*)USER_HEAP_START;
  heap_start_head->size = 4096 - sizeof(heap_header_t);
  heap_start_head->free = true;
  heap_start_head->next = NULL;
}

void split_block(heap_header_t* block, size_t requested_size) {

  size_t space_remaining = block->size - requested_size;

  if (space_remaining < sizeof(heap_header_t) + sizeof(uint8_t)) {
    return;
  }
  
  heap_header_t* new = (void*)((void*)block + requested_size + sizeof(heap_header_t));
  new->size = (block->size)-requested_size-sizeof(heap_header_t);
  new->free = true;
  new->next = block->next;

  block->size = requested_size;
  block->free = false;
  block->next = new;
}

heap_header_t* find_free_block(size_t requested_size) {
    heap_header_t* current_head = heap_start_head;
    while (current_head) {
        if (current_head->free && current_head->size >= requested_size) {
            return current_head;
        }
        current_head = current_head->next;
    }
    return NULL;
}

void expand_heap(size_t requested_size) {
  uintptr_t original_end = heap_end;
  heap_end = (uintptr_t)sbrk(requested_size);

  heap_header_t* traversal_head = heap_start_head;
  while (traversal_head) {
    if (!traversal_head->next && !traversal_head->free) {
      heap_header_t* new_header = (heap_header_t*)original_end;

      new_header->free = true;
      new_header->next = NULL;
      new_header->size = requested_size - sizeof(heap_header_t);

      traversal_head->next = new_header;
      break;
    } else if(!traversal_head->next && traversal_head->free) {
      traversal_head->size += requested_size;
      break;
    }
    traversal_head = traversal_head->next;
  }
}

void *malloc(size_t bytes) {
  if (bytes == 0)
    return NULL;
  void *result;
  heap_header_t *curr;

  if (!heap_start_head)init_heap();  

  heap_header_t* block = find_free_block(bytes);
  if (block) {
    if (block->size != bytes) split_block(block, bytes);
    block->free = false;
    result = (void*)((uintptr_t)block + sizeof(heap_header_t));
  } else {
    expand_heap(bytes);
    return malloc(bytes);
  }
  return result;
}

void merge_blocks(heap_header_t* block) {
  if (!block->next || !block->next->free) return;
  heap_header_t* block_to_merge = block->next;
  
  block->size += block_to_merge->size + sizeof(heap_header_t);
  block->next = block_to_merge->next;
  return merge_blocks(block);
}

heap_header_t* get_header(void* ptr) {
  return (heap_header_t*)((uintptr_t)ptr - sizeof(heap_header_t));
}

void free(void* ptr) {
  if (!ptr) return;
  heap_header_t* block_to_free = get_header(ptr);
  block_to_free->free = true;
  merge_blocks(block_to_free);
}

void* memcpy(void* dest, const void* src, size_t n) {
  uint8_t* d = (uint8_t*)dest;
  const uint8_t* s = (const uint8_t*)src;

  for (size_t i = 0; i < n; i++) {
    d[i] = s[i];
  }

  return dest;
}

void* memmove(void* dest, const void* src, size_t n) {
  uint8_t* d = (uint8_t*)dest;
  const uint8_t* s = (const uint8_t*)src;

  if (d == s || n == 0) return dest;

  if (d < s) {
    //safe to copy won't overwrite the source
    for(size_t i = 0; i < n; i++) {
      d[i] = s[i];
    }
  } else {
    // copy backwards so source isn't overwritten
    for (size_t i = n; i > 0; i--) {
      d[i - 1] = s[i - 1];
    }
  } 
  return dest;
}

void* memset(void* ptr, uint8_t c, size_t n) {
  uint8_t* p = ptr;
  while (n--) {
    *p++ = (c);
  }
  return ptr;
}

int memcmp(void* ptr1, void* ptr2, size_t n) {
  size_t i;
  uint8_t* p1 = (uint8_t*)ptr1;
  uint8_t* p2 = (uint8_t*)ptr2;
  int compare_status = 0;

  if (ptr1 == ptr2) return compare_status;

  while (n > 0) {
    if (*p1 != *p2) {
      compare_status = (*p1 > *p2) ? 1 : -1;
      break;
    }
    n--;
    p1++;
    p2++;
  }
  return compare_status;
}

void* calloc(size_t n, size_t size) {
  size_t total = n * size;
  void *ptr = malloc(total);

  if (ptr) memset(ptr,0,total);
  return ptr;
}

void* realloc(void* ptr, size_t new_size) {
  if (!ptr) return malloc(new_size);

  if (new_size == 0) {
    free(ptr);
    return NULL;
  }

  heap_header_t* old_header = get_header(ptr);

  void* new_ptr = malloc(new_size);

  if (new_ptr == NULL) return NULL;

  size_t copy_size = old_header->size;

  if (copy_size > new_size) copy_size = new_size;

  memcpy(new_ptr, ptr, copy_size);
  free(ptr);
  return new_ptr;
}
```

#### Differences with the kernel allocator

Most of this code was ripped directly from the kernel heap allocator. Let's look at the 
differences so we aren't being redundant.

##### Initialization

The kernel had to map its own physical frame here, but the user space initialization is 
a lot smaller, this is because we assume that the memory already exists, and then we create
the heap header.

##### Splitting blocks

The calculation of the new block's size and start is slightly different, 
they're the same calculation, done in slightly different ways.
Another difference here is that the `block->free = false` is done inside, 
but this is inside `kmalloc` for the kernel.

##### Expansion

This is where the largest differences are. For expansion, the kernel allocates
and maps its own pages to expand the space of the heap. Code like this may be required

```c
uint32_t required_pages =
    (required_size + 4096 - 1) / 4096;

void* new_page_physical_start = alloc_frame();

map_page(kernel_directory,
         heap_end,
         (uintptr_t)new_page_physical_start,
         PAGE_PRESENT | PAGE_WRITABLE);
```

The user heap does this all within one line:

```c
heap_end = (uintptr_t)sbrk(requested_size);
```

`SBRK` is the interface through which a user process requests the kernel to increase its
heap region.

The second difference is that the kernel expands via 4KiB pages, the user heap does not do this and expands 
by a specified size that is exactly the requested amount. Under the hood `SBRK` does allocate 
whole pages, but only makes the requested space available.

Difference three involves the way that `heap_end` gets handled. The kernel heap
advances the end by a single page after allocating. With user code, the old end
gets saved because this is where the new free block's header will go:

```c
heap_header_t* new_header = (heap_header_t*)original_end;
```

The final difference when we want to extend the latest block in the linked list. The kernel does this
by page size, the user space does it by requested size.

##### `kmalloc` vs `malloc`

User allocation initializes itself in `malloc`'s first call. The kernel does not do this
and assumes that it has already been initialized

#### New Additions

Now it's time to cover the new functions that we have written.
Not much needs to be said for most of the memory manipulation functions as they're all 
primitive in behaviour.

##### `memcpy`

Copies memory from one address to the next, in the function 
we convert the void pointers to byte pointers so we can iterate over them.
In the for loop we just set `d[i] - s[i]` and return the pointer to the destination.

##### `memove`

Does the same as the previous essentially, if you don't understand how copying
backwards protects the source, take a look at this diagram:

```
Initial memory:

Address →    100   101   102   103   104   105   106
             ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┐
             │  A  │  B  │  C  │  D  │  E  │  F  │  G  │
             └─────┴─────┴─────┴─────┴─────┴─────┴─────┘
              \_____________________/
                    source

                   \_____________________/
                         destination
                         starts at 102
```

This diagram shows that the source contains data: `ABCD` and the 
destination contains `BCDE` most of the source exists within the destination.
If we copied this forward by first doing `destination[0] = source[0]` then B would
be overwritten and the copy would not work correctly.
This is why we copy backwards.  

##### `memset`

Set `n` entries to value `c` at pointer `ptr`.

##### `memcmp`

This function simply iterates until n is equal to 0. For each de-referenced byte we check
whether one is greater than the other, if they are different we break from the loop and return
1 if `*p1 > *p2` and -1 if `*p1 < *p2`.

##### `calloc`

This function is typically used for dynamically allocating arrays, it's just a nice wrapper.
In the code we calculate the total size that gets allocated and use `malloc`.
After that, if `malloc` was successful, we use `memset` to initialize all the data
to 0.

##### `realloc`

After the safety checks
the old header is retrieved and `malloc` is used to allocate memory with the new size. 
After that we use `memcpy` to copy the previous data to the new allocation, we also ensure we don't overflow
when writing the data for instances where we re-allocate to a smaller size.

### String manipulation

Here's the header:

```c
#ifndef STRING_H
#define STRING_H

#include <stddef.h>

size_t strlen(const char *str);
int strcmp(const char *str1, const char *str2);
int strncmp(const char *str1, const char *str2, size_t n);
char* strcpy(char* dest, const char *src);
char* strcat(char* dest, const char *src);
char* strchr(const char *str, int c);
char* strstr(const char *text, const char *search);


#endif
```

And then here's the implementation code:

```c
#include "user/libc/string.h"

size_t strlen(const char *str) {
  size_t len = 0;

  while (str[len] != '\0') len++;

  return len;
}

int strcmp(const char *str1, const char *str2) {
  while (*str1 && *str1 == *str2) {
    str1++;
    str2++;
  }
  return *str1 - *str2;
}

int strncmp(const char *str1, const char *str2, size_t n) {
  size_t i = 0;

  while (i < n) {
    char c1 = str1[i];
    char c2 = str2[i];

    if (c1 != c2) return c1 - c2;

    if (c1 == '\0') return 0;

    i++;
  }

  return 0;
}

char* strcpy(char *dest, const char *src) {
  while ((*dest++ = *src++) != '\0');
  return dest;
}

char* strcat(char* dest, const char* src) {
  while (*dest)dest++;
  while ((*dest++ = *src++) != '\0');
  return dest;
}

char* strchr(const char *str, int c) {
  while (*str) {
    if (*str == c) return (char *)str;
    str++;
  }

  if (c == '\0') return (char *)str;

  return NULL;
}

char* strstr(const char *text, const char *search) {
  size_t search_length;
  size_t i;
  size_t j;

  search_length = strlen(search);

  for (i = 0; text[i] != '\0'; i++) {
    for (j = 0; j < search_length; j++) {
      if (text[i + j] == '\0') break;

      if (text[i + j] != search[j]) break;
    } 

    if (j == search_length) return (char*)&text[i];
  }

  return NULL;
}
```

#### `strlen`

This iterates over the string until `'\0'` gets found, incrementing the length for each
loop. This is the value that get returned.

#### `strcmp`

Loops while the de-referenced str1 is valid, and both de-refernced values 
are equal to one another the body of the loop just increments both pointers to scan
through the string. We then return the difference at the end if the string is the same, 
this will just be zero, if there has been a difference in char then this will be the ASCII difference.

`strncmp` does the same but just for an n set of elements

#### `strcpy`

Copies the source to destination by looping and having the condition be the assignment not being 
equal to `'\0'`.

#### `strcat`

Same as previous, but before doing anything we iterate to the end of destination.

#### `strstr`

This uses a nested for loop in order to search if one string is contained within another.

### Output

The header:

```c
#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdarg.h>
#include "syscalls.h"

void printf(char* format, ...);
int putchar(int c);
int puts(char* str);

#endif
```

And the implementation:

```c
#include "user/libc/output.h"

int putchar(int c) {
  return write(1, &c, 1) ? 1 : -1;    
}

int puts(char* str) {
  while(*str != '\0') {
    if (!putchar(*str)) return -1;
    str++;
  }
  return 1;
}

void print_num(int num) {
  if (num == 0) {
    putchar('0');
    return;
  }

  if (num < 0) {
    putchar('-');
    num = -num;
  }

  char buffer[10];
  int i = 0;

  while (num > 0) {
    buffer[i++] = (num % 10) + '0';
    num /= 10;
  }

  while (i--) {
    putchar(buffer[i]);
  }

}

void printf(char* format, ...) {
  char *traverse;
  unsigned int i;
  char *s;

  va_list args;
  va_start(args, format);

  while (*format) {
    if (*format == '%') {
      format++;
      if (*format == 'c') {
        char c = va_arg(args, int);
        putchar(c);
      } else if (*format == 's') {
        char *str = va_arg(args, char*);
        puts(str);
      } else if (*format == 'd') {
        int num = va_arg(args, int);
        print_num(num);
      } else if (*format == '%') {
        putchar('%');
      } else {
        //unknown format, so print the raw
        putchar('%');
        putchar(*format);
      }
    } else {
      putchar(*format);
    }
    format++;
  }
  va_end(args);
}
```

`puts` and `putchar` do not require explanation. Our version of `printf` is simple
and does not handle all cases that are typically handled by `printf`. The implementation 
I've made only handles the following format specifiers: `%c, %s, %d`. The biggest limitation here
is that floats and doubles cannot be worked with. If you wish to make any software that prints out
these data types, you may wish to add functionality for these format specifiers.

`<stdarg.h>` allows us to make functions that take variadic arguments, these are arguments
that allow functions to accept a variable number of arguments, this is indicated by an ellipsis
(...) in the function declaration.

`va_list args` creates a variable that keeps track of where the next variadic 
argument is. `va_start(args, format)` initializes it and basically tells the compiler
"Initialize `args` so that it can start retrieving the arguments that come after `format`". 
`va_arg(args, int)` retrieves the next argument and interprets it as an integer. This is 
all we need to know about variadic arguments to create this function.

After you understand variadic arguments, it all becomes pretty simple, the format
is iterated over and if a format specifier gets found we appropriately take in the variadic 
argument and print it. If there is no format specifier, print the character.

#### `print_num`

This is a helper function we make specifically for printing numbers. If the number is 0, we output
0, if it less than zero, we print `'-'` before continuing and flip the sign of the number.
We then loop while the number is greater than 0, and write each number to a string before returning it by
getting the remainder of a division by 10.
A constraint with this is we cannot print an integer greater than 10 digits. Keep that in mind if you 
ever try to print an integer with more than 10 digits (this will only happen if you try to handle 
64-bit integers).

### Character manipulation

The final functionality being added to our libc. I will not be walking through these functions as they
are painfully simple, just have a look at my code.

Header:

```c
#ifndef CHARS_H
#define CHARS_H

int isalpha(int c);
int isdigit(int c);
int isalnum(int c);
int isspace(int c);

int islower(int c);
int isupper(int c);

int tolower(int c);
int toupper(int c);

#endif
```

Implementation:

```c
#include "user/libc/chars.h"

int isalpha(int c) {
  return ((c >= 'a' && c <= 'z') ||
      (c >= 'A' && c <= 'Z'));
}

int isdigit(int c) {
  return (c >= '0' && c <= '9');
}

int isspace(int c) {
  return (c == ' ' ||
      c == '\t' ||
      c == '\n' ||
      c == '\v' ||
      c == '\f' ||
      c == '\r');
}

int islower(int c) {
  return (c >= 'a' && c <= 'z');
}

int isupper(int c) {
  return (c >= 'A' && c <= 'Z');
}

int tolower(int c) {
  if (isupper(c)) return c + ('a' - 'A');
  return c;
}

int toupper(int c) {
  if (islower(c)) return c - ('a' - 'A');
  return c;
}
```

## C archives

When linking our C standard library in with the user space programs we create in future, we will want
to link our libc in with our programs. This causes an issue when we link them all like regular C programs.
For example if a C program we write only uses `printf` we don't want to
also link in other un-needed functionality. This becomes especially important when we make
our file system and want to store programs on it.

When you set up your cross-compiler you may have access to the command `i686-elf-ar`. This program
is used to making something called archives.
An archive is a library of object files where it only links required object files when linked in with
user space programs.

Here are my Makefile rules for compilation:

```makefile
AR = i686-elf-ar

USER_LIB = $(BUILD_DIR)/libc.a

USER_LIB_C_FILES = $(shell find user/libc -name '*.c')
USER_LIB_C_OBJECTS = $(USER_LIB_C_FILES:%.c=$(BUILD_DIR)/%.o)

USER_LIB_ASM_FILES = $(shell find user/libc -name '*.asm')
USER_LIB_ASM_OBJECTS = $(USER_LIB_ASM_FILES:%.asm=$(BUILD_DIR)/%.asm.o)


$(USER_LIB): $(USER_LIB_C_OBJECTS) $(USER_LIB_ASM_OBJECTS)
	mkdir -p $(dir $@)
	$(AR) rcs $@ $^
```

This is then linked in when compiling use space programs like so:

```makefile
$(SHELL_ELF): $(BUILD_DIR)/user/programs/shell.o  $(USER_LIB)
	mkdir -p $(dir $@)
	$(LD) -m elf_i386 -T $(USER_LINKER) $^ -o $@

$(SHELL_BIN): $(SHELL_ELF)
	$(OBJCOPY) -O binary $< $@
```
