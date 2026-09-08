# Part XIII: The Shell

## What do we need to do?

We now have a way to get `.bin` programs from the host operating system into the
custom one. Now, the next order of business is to get programs from the 
filesystem and load them into memory. This won't be an external program like
MKFS, and we are back to expanding the custom OS and kernel. We also have
a nice interface for reading from the filesystem, so the interface we are making,
called a loader, will be short and simple.

After the loader gets made, the next step is to write the shell as a user space program.
This will include adding new syscalls to the operating system, that access these kernel utilities:
-   `write` and `read` syscalls need to be modified to use file descriptors.
-   `open` and `close` syscalls need to be made for the filesystem.
-   A syscall needs to be made for general filesystem operations that take in a path,
    like: `ls`, `mkdir`, `touch`, `rm` and `run`.

For the shell, new data needs to be introduced into `process_t`. This is data for running
programs as when a user space program runs another one, it needs to wait until the program
it's running is finished before continuing. Basically, When the shell runs another program it must be 
blocked until the child program finishes execution.

There also need to be modifications to `keyboard.c`, whenever we press the enter key, we want
to stop reading from `stdin`, backspaces must also be handled to remove from whatever is in
the process's buffer, otherwise backspace will only be visual.

## Loading and syscalls

I will make a small header here for the loader:
```c
#ifndef LOADER_H
#define LOADER_H

#include "tasks/procman.h"

process_t *load_program(const char* path);

#endif
```

Then the implementation file is here:

```c
#include "tasks/loader.h"
#include "filesystem/fs_manager.h"
#include "filesystem/fs.h"
#include "tasks/procman.h"
#include "memory/pmm.h"
#include "kernel/mappings.h"
#include "kernel/kernel_utils.h"

process_t* load_program(const char* path) {
  int32_t fd = fs_open(path);
  uint32_t inode_num = fs_fd_to_inode(fd);

  fs_inode_t file_inode;

  if (!fs_read_inode(inode_num, &file_inode))
    return NULL;

  size_t file_size = file_inode.size;

  uint8_t buffer[file_size];
  if (fs_read(fd, buffer, file_size) == -1)
    return NULL;

  process_t* user_proc = create_process((void*)USER_CODE_BASE, PROCESS_USER);

  for(size_t i = 0; i < ((file_size + 4095) / 4096); ++i) {
    void* code_frame = alloc_frame(); 

    
    //write to frame
    size_t offset = i * 4096;
    size_t bytes = file_size - offset;
    if (bytes > 4096)
      bytes = 4096;
    memcpy(code_frame, buffer + offset, bytes);

    map_page(
      user_proc->page_directory,
      USER_CODE_BASE + (4096 * i),
      (uintptr_t)code_frame,
      PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER
    ); 
  }

  fs_close(fd);

  return user_proc;
}
```

This is the point where you can remove the previous code we had that copied the `user_test` code into
an allocated frame when we first set up the user space. This is the much better replacement.
In this code we first open the given file and get it's inode number. The file size is the most important
thing to retrieve here as that tells us how much data we need to allocate. We then bring the fill binary
file into memory by defining a buffer with its size and using `fs_read`. A new user process is created, and then
the loop copies data in the buffer to a frame in memory, each frame that is copied to is then given its own
virtual mapping.

Now we can load programs into memory, we also need to make modifications to `process_t` as I said,
additions are marked with `[+]`:

```c
typedef struct process {
[+] uint32_t pid_waiting_for;
[+] uint32_t parent_pid;

    uintptr_t user_heap_end;
    uint32_t heap_pages_allocated;

    uint32_t wake_tick;
    process_reading_state_t reading_state;

    uint32_t pid;
    
    process_registers_t regs;

    process_states_t state;
    process_type_t type;

    struct process* next;

    page_directory_t* page_directory;

    void* kstack;
    void* ustack;
} __attribute__((packed)) process_t; 
```

The way we use these two new variables is included in our `syscall_handler`. I'll just give you the
full function, as a lot has changed:

```c
void syscall_handler(registers_t* regs) {
    int fd;
    uint32_t buffer;
    size_t count;
    switch (regs->eax) {
        case SYSCALL_EXIT:
            // wake up parent
            process_t* parent = find_process_by_pid(current_process->parent_pid);

            if (parent != NULL && parent->pid_waiting_for == current_process->pid) {
                parent->pid_waiting_for = 0;
                parent->state = PROCESS_READY;
            }

            //exit
            current_process->state = PROCESS_TERMINATED;
            context_switch(current_process, get_next_process(), regs);
            break;
        case SYSCALL_GETPID:
            regs->eax = current_process->pid;
            break;
        case SYSCALL_YIELD:
            context_switch(current_process, get_next_process(), regs);
            break;
        case SYSCALL_SLEEP:
            current_process->wake_tick = timer_get_ticks() + regs->ebx;
            current_process->state = PROCESS_SLEEPING;
            context_switch(current_process, get_next_process(), regs);
            break;
        case SYSCALL_WRITE:
            fd = regs->ebx;
            buffer = regs->ecx;
            count = regs->edx;

            
            if (fd < 1) {
                regs->eax = -1;
                break;
            }
            if (fd > 2) {
                regs->eax = fs_write(fd, (void*)buffer, count);
                break;
            }

            ((char*) buffer)[count] = '\0';
            vga_text_write(&terminal, (char*)buffer);
            
            regs->eax = (int)count;
            break;
        case SYSCALL_READ:
            fd = regs->ebx;
            buffer = regs->ecx;
            count = regs->edx;

            if (fd < 0 || fd == 1) { 
              regs->eax = -1;
              return;
            }
            if (fd > 2) {
                regs->eax = fs_read(fd, (void*)buffer, count);
                break;
            }

            current_process->state = PROCESS_BLOCKED;
            current_process->reading_state.buffer = (void*)regs->ecx;
            current_process->reading_state.size  = regs->edx;
            current_process->reading_state.count = 0;
            context_switch(current_process, get_next_process(), regs);
            //keyboard is treated as stdin, so need to block until we recieve that data
            //need to block the process until we wait for input

            break;
        case SYSCALL_SBRK:
            current_process->user_heap_end += regs->ebx;
            
            //allocate more pages
            while (((current_process->user_heap_end + 4096) - USER_HEAP_START) / 4096 
                > current_process->heap_pages_allocated){
                map_page(current_process->page_directory,
                    USER_HEAP_START + (current_process->heap_pages_allocated++ * 4096),
                    (uintptr_t)alloc_frame(),
                    PAGE_PRESENT | PAGE_USER | PAGE_WRITABLE
                );
                    
            }

            regs->eax = current_process->user_heap_end;


            break;
        case SYSCALL_FSOPS:
            char* path = (char*)regs->ecx; 
            switch (regs->ebx) {
                case FS_LS:
                    fs_ls(path);
                    break;
                case FS_MKDIR:
                    fs_mkdir(path);
                    break;
                case FS_TOUCH:
                    fs_touch(path);
                    break;
                case FS_RM:
                    fs_rm(path);
                    break;
                case FS_RUN:
                    process_t* child = load_program(path);

                    child->parent_pid = current_process->pid;

                    current_process->pid_waiting_for = child->pid;
                    current_process->state = PROCESS_BLOCKED;
                    context_switch(current_process, get_next_process(), regs);
                    break;
            }
            break;
        case SYSCALL_OPEN:
            regs->eax = fs_open((char*)regs->ebx);
            break;
        case SYSCALL_CLOSE:
            fs_close(regs->ebx);
            break;
        default:
            vga_text_writeline(&terminal, "syscall not found");
            break;
    }
    return;
}
```

The largest addition here is `SYSCALL_FSOPS`, because there are a multitude of syscalls that only require
the path as a parameter, I just bunched these up into one large syscall. `ls`, `mkdir`, `touch`, and `rm` are all
obvious in how they work. But with the `run` syscalls, we must create a child process by using the `load_program`
function that we just wrote. The `current_process` must store the PID of the process it's waiting for and be set
to blocked. A context switch must also happen, so the current process doesn't continue.

The parent process would be unblocked here:
```c
        case SYSCALL_EXIT:
            // wake up parent
            process_t* parent = find_process_by_pid(current_process->parent_pid);

            if (parent != NULL && parent->pid_waiting_for == current_process->pid) {
                parent->pid_waiting_for = 0;
                parent->state = PROCESS_READY;
            }

            //exit
            current_process->state = PROCESS_TERMINATED;
            context_switch(current_process, get_next_process(), regs);
            break;
```

When a program exits, we find the parent by PID, restore it, and then end the current one.
(You may wish to also initialize `parent_pid` to 0 in `create_process` so we aren't checking
against an uninitialised variable.)

That's all for how loading works, now let's look at the extra syscall modifications we made in 
`syscall_handler`.

### Syscall additions

`read` and `write` can now interface with the filesystem via checking if the file descriptor is above
2 and then calling `fs_read`/`fs_write` for the respective syscalls. `EAX` is also used to return
the number of bytes written and read.

The next change is the addition of `SYSCALL_OPEN` and `SYSCALL_CLOSE`. `SYSCALL_OPEN` returns
the file descriptor.

Then these:
```c
void fs_ops(uint8_t operation, char* path) {
  syscall(SYSCALL_FSOPS, operation, (uint32_t)path, 0);
}

int open(const char* path) {
  return syscall(SYSCALL_OPEN, (uint32_t)path, 0, 0);
}

void close(int fd) {
  syscall(SYSCALL_CLOSE, fd, 0, 0);
}
```
Must get added to `syscalls.c` and their respective definitions to `syscalls.h`.

## Keyboard changes

There's one more thing we must add before actually writing the shell. This is the keyboard, 
we need for the keyboard to be able to handle enter (which makes us stop reading), and backspace
which removes a character in the buffer. Here's the full code for the altered function:

```c
void keyboard_handler () {
    uint8_t scancode = inb(PS2_DATA);
    
    if (keyboard_modifier_keys(scancode)) {
        return;
    }
    if (scancode == EXTENDED_SCANCODE) {
        keyboard_extended_scancodes();
        return;
    }
    if (scancode & KEY_RELEASED) {
        return;
    }

    if (scancode >= 128) {
        return;
    }

    char c[2];
    c[0] = keymap[scancode];
    c[1] = '\0';

    switch (c[0]) {
        case 27: 
            vga_text_clear(&terminal);
            break;
        case '\t':
            vga_text_write(&terminal, "    ");
            break;
        case '\b':
            vga_text_backspace(&terminal);
        case '\n':
        default:
            if (shift_pressed) {
              c[0] = shift_keymap[scancode];
            }
            if (c[0] != '\b')
              vga_text_write(&terminal, c);
            if (!c[0]) return;

            process_t* traversal_process = process_head;
            while (traversal_process) {
              if (traversal_process->state == PROCESS_BLOCKED && traversal_process->reading_state.size > 0) {
                void* buffer = traversal_process->reading_state.buffer;
                uint32_t size = traversal_process->reading_state.size;

                if (c[0] == '\b') {
                  if (traversal_process->reading_state.count)
                    traversal_process->reading_state.count--;
                  set_cr3((uintptr_t)traversal_process->page_directory);
                  ((char*)(buffer))[traversal_process->reading_state.count] = '\0';
                  set_cr3((uintptr_t)kernel_directory);
                } else {
                  //quite a crude way of doing this, (really only the context switcher should 
                  //be changing cr3), but it works
                  set_cr3((uintptr_t)traversal_process->page_directory);
                  ((char*)(buffer))[traversal_process->reading_state.count++] = c[0];
                  set_cr3((uintptr_t)kernel_directory);
                }


                if (traversal_process->reading_state.count >= size || c[0] == '\n') {
                  traversal_process->regs.eax = traversal_process->reading_state.count;
                  traversal_process->reading_state.count = 0;
                  traversal_process->reading_state.size = 0;
                  traversal_process->state = PROCESS_READY;
                }

              }

              traversal_process = traversal_process->next;
            }
            break;
    }
}
```

Most of the changed logic is within the default case. Both `'\n'` and `'\b'`
characters leak into the default case where most of the logic is handled.
For backspaces, we have this small snippet of code:
```c
                if (c[0] == '\b') {
                  if (traversal_process->reading_state.count)
                    traversal_process->reading_state.count--;
                  set_cr3((uintptr_t)traversal_process->page_directory);
                  ((char*)(buffer))[traversal_process->reading_state.count] = '\0';
                  set_cr3((uintptr_t)kernel_directory);
                } else {
```
Basically, if the count is not zero and the character is a backspace, we remove one character from
the buffer and put a null terminator on the end of it. If there is no backspace, we just do regular writing of characters.

Enter is handled here:
```c
                if (traversal_process->reading_state.count >= size || c[0] == '\n') {
                  traversal_process->regs.eax = traversal_process->reading_state.count;
                  traversal_process->reading_state.count = 0;
                  traversal_process->reading_state.size = 0;
                  traversal_process->state = PROCESS_READY;
                }
```
Where not only does the count reaching the size cause reading to end, but also the character being a 
new line causes the process to finish reading. The value we store in `EAX` is new, and it tells 
the caller of `read()` how many bytes (or characters) have been read.

That's all, now we can actually get into writing the shell:

## Writing the Shell

The shell follows something called a REPL (Read, Evaluate, Print, Loop). This sums up basically
everything we want the shell to do. We want it to read input, evaluate the input, print the output, and 
then loop this all again.

Let me show you all my code and then we can get into explaining it:

```c
#include "user/programs/shell.h"

#include "user/libc/syscalls.h"
#include "user/libc/output.h"
#include "user/libc/chars.h"
#include "user/libc/memory.h"
#include "user/libc/string.h"


void start(void) {  
  //REPL
  for (;;) {
    printf("> ");
    //Read 
    char input[50];
    read(0, input, 50); 
    input[strlen(input) - 1] = '\0';

    //Eval
    size_t word_count;
    char** words = tokenize_line(input, &word_count);
    //printf("we typed: %s, %d, %d, %s\n", input, strlen(input), word_count, words[0]);

    //Print
    if (!execute_command(words, word_count)) {
      printf("command failed\n");
    }

    //CLEANUP
    memset(input, 0, strlen(input));
    word_count = 0;
    free(words);


    
  }
}

bool execute_command(char** words, size_t word_count) {
  if (word_count != 2) {
    return false;
  }

  int fd;

  if (strcmp(words[0], "ls") == 0) {
    fs_ops(FS_LS, words[1]);
  }
  else if (strcmp(words[0], "mkdir") == 0) {
    fs_ops(FS_MKDIR, words[1]);
  }
  else if (strcmp(words[0], "touch") == 0) {
    fs_ops(FS_TOUCH, words[1]);
  }
  else if (strcmp(words[0], "rm") == 0) {
    fs_ops(FS_RM, words[1]);
  } 
  else if (strcmp(words[0], "run") == 0) {
    fs_ops(FS_RUN, words[1]);
  }
  else if (strcmp(words[0], "cat") == 0) {
    fd = open(words[1]);

    if (fd < 0) {
      return false;
    }

    char buffer[128];
    int n;

    while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
      write(1, buffer, n);
    }
    printf("\n");

    close(fd);
  }
  else if (strcmp(words[0], "write") == 0) {
    fd = open(words[1]);

    if (fd < 0) {
      return false;
    }

    size_t capacity = 128;
    size_t length = 0;

    char* buffer = malloc(capacity);

    if (buffer == NULL) {
      close(fd);
      return false;
    }

    while (true) {
      char input[128];

      int n = read(0, input, sizeof(input));

      if (n < 0) {
        free(buffer);
        close(fd);
        return false;
      }

      if (n == 0) {
        break;
      }

      if (length + n > capacity) {
        while (length + n > capacity) {
          capacity *= 2;
        }

        char *new_buffer = realloc(buffer, capacity);

        if (new_buffer == NULL) {
          free(buffer);
          close(fd);
          return false;
        }

        buffer = new_buffer;
      }

      memcpy(buffer + length, input, n);
      length += n;

      if (input[n-1] == '\n') {
        break;
      }
    }

    if (length > 0 && buffer[length - 1] == '\n') {
      length--;
    }

    if (length > 0) {
      if (write(fd, buffer, length) != (int)length) {
        free(buffer);
        close(fd);
        return false;
      }
    }

    free(buffer);
    close(fd);
  }
  else {
    //command not recognised
    return false;
  }

  return true;
}


char** tokenize_line(const char* line, size_t *count_out) {
  const char* p = line;
  char **words;
  size_t words_i = 0;
  size_t capacity = 8;

  // limit of words in a line is 8
  words = calloc(capacity + 1, sizeof(char *));

  while (*p != '\0') {
    (*count_out)++;
    const char* start;

    while (isspace((unsigned char) *p)) {
      p++; 
    }

    if (*p == '\0') {
      break;
    }

    start = p;

    while (*p != '\0' && !isspace((unsigned char) *p)) {
      p++;
    }
    size_t len = p - start;
    char *word = malloc(len + 1);
    if (word == NULL) {
      return NULL;
    }

    memcpy(word, start, len);
    word[len] = '\0';
    if (word == NULL) {
      return NULL;
    }
    words[words_i++] = word;
  }
  return words;
}
```

The REPL we have is actually small, we first use `read` to read a string of maximum 50 characters
and then append a null terminator onto the end of it.
Then, we tokenize and **evaluate** the data via the `tokenize_line` function, which converts the single
string into an array of words with a word count.
Then after tokenizing, we execute the command and cleanup memory, ready for the next loop.
Let's also have a closer look into all the functions that facilitate the REPL:

### `tokenize_line`

As I said, this function converts a command line that has been read into 
an array of individual words. For example `ls /bin` may become 
`{"ls", "/bin"}` with a word count of 2.

In this function, `p` walks through the input string. `isspace` is then used to skip
whitespace and identify where words end. When we do `start = p`, `start` is used to record
the beginning of a word, then when we get to the end of one, a string 
for the word is then allocated using `malloc` and the word is copied into here. 

After each word is found, the pointer is then stored within `words`.

### `execute_command`

This is the meat and potatoes of the shell, most commands are simple, and the logic has already
been made for commands such as: `ls`, `mkdir`, `touch`, `rm`, and `run`. But `cat` and `write` are
a different story.

This function expects exactly two words, this is because the shell we are making is simple,
and will only be taking the word for the command, and the word for the path.
Let's go straight into explaining `cat` and `write`, as all other functions are simple.

#### `cat` 

This is the simpler of the two, all it does is `open()` the requested file, repeatedly `read()` chunks
into a buffer, `write(1, ...)` those bytes to `stdout`, and then finally closes the file.

#### `write`

Write does the opposite thing but is a little more complex. It `open()`s the file,
reads input from `stdin` using `read(0, ...)`, dynamically grows a buffer when necessary
using `realloc()`, removes the final new line (as hitting enter would finish the writing), writes
all accumulated data from the file, and then it frees the buffer and closes the file.

Write is basically a mini text editor, as when we run it, it intends to push all characters
typed until the enter key is hit.

## The final change to `kernel_main`

Then, the final touch in the whole of the guide should be the addition of:
```c
    load_program("/bin/shell");
```
To `kernel_main.c`. Then, the shell should be running.



<!-- 
-DEV NOTES-
-changes to the keyboard for backspace, returning on enter, and returning on read
-for running, need to block a process
-->

