# Part IX : Physical Memory Manager

This chapter marks a larger milestone in our OS development, this is
where we start developing one of our first major kernel subsystems, this
being memory management. Due to this subsystem being larger than previous
parts, the next 3 chapters (including this one) is dedicated to memory
management.
The 3 parts are split into:

-   Physical Memory Manager (PMM): Manages actual RAM
-   Virtual Memory Manager (VMM): Manages virtual addresses using paging
-   Kernel Heap: provides dynamic allocation (`kmalloc`/`kfree`)

The later two will be explained in due time. But for now lets start with
the PMM

## Where are we now?

Currently, our OS is a kernel that is loaded into memory by the
bootloader, the code can access memory, but only statically as defined
at compile time, so we can create and access variables and their size,
addresses and lifetime is all defined when we compile our OS. This is
fine. But we will soon need to implement things where the size is
unknown, which is where dynamic allocation comes in (which is what is
made in chapter 11). But before we can implement things such as `malloc`,
we need to know which memory is actually available for us to use. This
is where PMM comes in.

Suppose we have 8GB of ram in our virtual machine, this 8GB has some
things that already occupy the space, such as the BIOS and the kernel,
so we need to a way to know whether memory is used or not, so
essentially the PMM is just a database of our ram usage, at a high
level, think of it as:

```
Address        Status

0x00000000     Used
0x00100000     Used
0x00200000     Free
0x00201000     Free
0x00202000     Used
```

## How does the PMM work?

The PMM divides RAM into fixed size chunks called frames. A frame is
typically 4KiB, and every byte in ram belongs to a single frame.
The PMM treats each frame as either free or used, it doesn't really
care about what the frame contains, it only cares about who owns the
frame. With the PMM we really only want two functions. These being
`alloc_frame();` and `free_frame();` with the latter having a parameter
be the frame address and the prior returning the frame address.

## How will we make this?

The first thing we need to know is how much RAM the computer has. 
No hardware in the CPU that can tell us this, but the BIOS knows this
information. The bootloader can query the BIOS on startup for something
called a memory map which is just data which describes the regions of
memory. It is an array where every item has 3 fields. These being:

-   Base address: tells us where the region begins in physical memory
-   Length: tells us how long this region covers
-   Type: tells us what the memory is used for (so like whether it's
    free or not)

We will not be using the memory map for long, it's just a piece of
information that will allow us to initialize the PMM properly.

Second, we need to split memory up into 4096 byte long frames, we do
this by creating a data structure that we will use to track our frames.
The data structure we use will be a bitmap, this is just an array of
bits where every bit maps to a frame, and every zero means free and
every one means used.

The third step is to then fill out in the bitmap the used and free
memory, this is done in different ways for each part. For the kernel we
must use a linker script to know the start and end location, and then
the bootloader has known boot locations. Then everything else
would be usable theoretically.

Now after all these steps we can make `allocate_frame()` which just
iterates the bitmap until a 0 is found. And then `free_frame()` just
indexes and then frees the frame. And then this is fully how the PMM
works.

## The implementation

Our implementation is simple in theory, we just have an init function,
our allocation and freeing functions, and some helper functions. But the
init function is long and complicated because it involves thinking
deeply about how our memory is structured and managing our large bitmap.

Before we go into making the PMM we must edit our bootloader code a bit
to pass the memory map to C. Here is our code to do so:

```x86asm
start:
    mov ax, 07C0h
    mov ds, ax

    mov si, title_string
    call print_string

    mov si, message_string
    call print_string

    call store_memory_map

    call load_kernel_from_disk
    jmp 0900h:0000 ; gives control to the kernel by jumping to it's starting point.

store_memory_map:
    xor ax, ax
    mov ds, ax ; in order to get exact addresses and not relative

    xor ebx, ebx ; first call
    xor bp, bp ; will store entry count

    mov di, memory_map_buffer ; safe memory location to write the map to
next_entry: 
    mov ax, 0
    mov es, ax
 
    mov eax, 0xE820
    mov edx, 0x534D4150 ;signiture that says we are requesting E820 memory map service
    mov ecx, 24

    int 15h

    jc done ; jump if there was an error

    add di, 24
    inc bp

    test ebx, ebx
    jnz next_entry
done:
    mov [memory_map_entries], bp
    mov ax, 07C0h
    mov ds, ax
    ret
```

and also we define these:

```x86asm
memory_map_entries equ 0x4FFC
memory_map_buffer equ 0x5000
```

The `memory_map_entries` address is where we will store the number of
memory maps and the `memory_map_buffer` is where we will store the actual
memory map.
Our algorithm here is a loop that iterates through the memory map
entries that are received each interrupt call and pastes them to memory.

At the start before everything we set `DS` to 0, this is because we want
the exact addresses and don't want to factor in the offset used by
memory segmentation. We set `EBX` to 0 as this register is used to define
which entry of the memory map we want to receive. `BP` is similar and will
increment for every **valid** entry found in the list. `DI` is set to the
memory address of the buffer as it is the destination index. We also set
`ES` to 0 too, as the data is written to ES:DI we set this every loop as
it's possible the BIOS can change this.

We then set `EAS` and `EDX` to values to signify that we are requesting the
memory map. `ECX` is set to the size in bytes of the memory map entry we
are requesting. We then call 15h, if the carry flag is set we jump to
the done label as there would have been an error.
Next we add 24 to `DI` as this is where we store the next entry, and we
increment `BP` of course. We then use test to check if `EBX` is 0. And if it
isn't, we go to the next entry. This is because `int 15h` sets EBX to 0
if we are at the final entry.

In our done label we move bp to the memory address for the number of
entries, then we move our previous data entry back

### The Header code

Now let's look at our header file:

```c
#ifndef PPM_H
#define PPM_H

#include 
#include 

#define FRAME_SIZE 4096
#define BITMAP_BASE 0x100000

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t attributes;
} __attribute__((packed)) memory_map_entry_t;

void init_pmm();

void bitmap_set_frame(uint32_t frame_index);
void bitmap_clear_frame(uint32_t frame_index);
void print_bitmap_summary();

void* alloc_frame();
void free_frame(void* frame_address);

#endif
```

We define the intended frame size and the memory address for the bitmap
for the frames. We then have a struct for our memory map entry. And then
our functions are pretty basic. We have `init_pmm()`. We then have
internal functions for setting and clearing frames, and then for
printing the bitmap when testing. Not only that, 
but we then have our functions to be used externally as we discussed before.

### The Implementation file

This is our largest section of code, i will first cover the init
function which is pretty long to get through and is the most complex
thing we have made in our kernel so far. First in our code we define
these globally:

```c
#include "pmm.h"
#include "../kernel/vga_text.h"

extern vga_text terminal;

uint32_t* bitmap = (uint32_t*)BITMAP_BASE;
uint32_t total_frames = 0;
uint32_t bitmap_entries = 0;
extern char _kernel_start;
extern char _kernel_end;
```

This defines the bitmap, total frames, number of entries there are in
the bitmap, and also we have 2 labels that we will use to get the start
and the end of the kernel from our linker file. I will show you the
`init_pmm` full function and then go into explaining it now:

```c
void init_pmm() {
    uint16_t map_entry_count = *(uint16_t*)0x4FFC;
    memory_map_entry_t* memory_map = (memory_map_entry_t*)0x5000;
    
    vga_text_write(&terminal, "Entries: ");
    vga_text_write_hex(&terminal, map_entry_count);
    vga_text_writeline(&terminal, "");

    for (uint16_t i = 0; i < map_entry_count; i++) {
        // Format: #0: B:0x00000000 L:0x00000000 T:0x01
        vga_text_write(&terminal, "#");
        vga_text_write_dec(&terminal, i);
        vga_text_write(&terminal, " B:");
        vga_text_write_hex(&terminal, memory_map[i].base);
        vga_text_write(&terminal, " L:");
        vga_text_write_hex(&terminal, memory_map[i].length);
        vga_text_write(&terminal, " T:");
        vga_text_write_hex(&terminal, memory_map[i].type);
        vga_text_writeline(&terminal, "");
    }

    uint64_t max_usable_address = 0;

    // find highest usable physical RAM address to calc max frames
    for (uint16_t i = 0; i < map_entry_count; i++) {
        if (memory_map[i].type == 1) {
            uint64_t highest_access = memory_map[i].base + memory_map[i].length;
            if (highest_access > max_usable_address) max_usable_address = highest_access;
        }
    }

    // calculate bitmap dimensions
    total_frames = max_usable_address / FRAME_SIZE;
    bitmap_entries = (total_frames + 32 - 1) / 32; //always round up
                                                    //
    // set all regions to reserved for safety
    for (uint32_t i = 0; i < bitmap_entries; i++) {
        bitmap[i] = 0xFFFFFFFF;
    }

    //mark usable regions as free using the bitmap
    for(uint32_t i = 0; i < map_entry_count; i++) {
        if (memory_map[i].type == 1) { // free memory 
            uint64_t starting_frame_index = (memory_map[i].base / FRAME_SIZE);
            uint64_t frame_length = memory_map[i].length / FRAME_SIZE;
            for (uint32_t j = starting_frame_index; j < starting_frame_index + frame_length; j++) {
                bitmap_clear_frame(j);
            }
        }
    }

    //protect kernel frames, bitmap and bootloader
    bitmap_set_frame(0); //protect bios data

    //kernel 
    uint32_t kernel_frame_index = (uint32_t)&_kernel_start / FRAME_SIZE;
    uint32_t kernel_frame_end = (uint32_t)&_kernel_end / FRAME_SIZE;

    for (; kernel_frame_index < kernel_frame_end; kernel_frame_index++) {
        bitmap_set_frame(kernel_frame_index);
    }

    //bitmap
    uint32_t bitmap_frame_start = BITMAP_BASE / FRAME_SIZE;
    uint32_t bitmap_size_bytes = bitmap_entries * sizeof(uint32_t);
    uint32_t bitmap_frame_count = (bitmap_size_bytes + FRAME_SIZE - 1) / FRAME_SIZE;
    for(uint32_t i = 0; i < bitmap_frame_count; i++) {
        bitmap_set_frame(bitmap_frame_start + i);
    }


    // protect bootloader
    bitmap_set_frame(0x7C00 / FRAME_SIZE);

    // protect video memory
    uint32_t video_frame_index = 0xA0000 / FRAME_SIZE;
    uint32_t video_frame_end = 0xFFFFF / FRAME_SIZE;
    for (; video_frame_index < video_frame_end; video_frame_index++) {
        bitmap_set_frame(kernel_frame_index);
    }

    print_bitmap_summary();

}
```

Foremost we retrieve the memory map and the number of entries from
memory, and then we print out the memory map for debug purposes. We then
define the max usable address, after this we iterate through the memory
map checking if the entry is type 1 (which means free). If it's free we
add the base and length and store the highest version of this value. The
max usable address is used to get the total size in memory and calculate
the number of frames.

We then define the `total_frames` by dividing the maximum address by the
size of each frame. From this we divide the total frames by 32 and
always round up to have the number of 32bit entries in the
bitmap. After this we set all regions in the bitmap to used. This is
done for safety purposes, it's much better to assume everything is used
than everything being free.

Next, we iterate through our memory map and then convert the addresses
to the index in the bitmap by dividing by the frame size on memory map
entries that are marked as free. This is used to set those bits that
have free frames to free. We also use one of our helper functions, these
are defined as such:

```c
void bitmap_set_frame(uint32_t frame_index) {
    bitmap[frame_index / 32] |= (1 << (frame_index % 32));
}
void bitmap_clear_frame(uint32_t frame_index) {
    bitmap[frame_index / 32] &= ~(1 << (frame_index % 32)); 
}
```

These functions just use bit logic to set and clear bits in the bitmap.

After setting the free memory it's time to protect the specific parts
our OS. First we protect the first frame as this has some BIOS data we
need. Next we need to edit our linker file to get the start and
the end of the kernel. This is our new linker:

```
ENTRY(start)

SECTIONS
{
    . = 0x9000;

    _kernel_start = .;

    .text :
    {
        *(.text)
    }

    .data :
    {
        *(.data)
    }

    .bss :
    {
        *(.bss)
    }

    _kernel_end = .;
}
```

This is pretty simple, we just get our kernel start at `0x9000`, and then
after all our code we set the kernel end. From these addresses we can
divide by our frame size to get our frame index and then after that we
iterate and set our bitmap to protect our kernel.
Next we follow a similar pattern to do the same thing for the location
in memory for our bitmap. Protecting the bootloader is simple as this is
just 512 bits at `0x7C00`. We must also protect the video memory for VGA
so we do that too.

That's the end of our init function, I have written this section of
code that I call at the end in order to visualize our bitmap and make
sure it's somewhat correct:

```c
void print_bitmap_summary() {
    vga_text_write(&terminal, "BITMAP: ");
    
    uint32_t total_free_frames = 0;
    uint32_t total_used_frames = 0;
    
    // Get initial state of Frame 0: 1 = reserved, 0 = free
    uint32_t current_state = (bitmap[0] & 1) ? 1 : 0; 
    uint32_t current_run_start = 0;

    // 1. Scan the entire bitmap to print consecutive blocks
    for (uint32_t f = 0; f < total_frames; f++) {
        uint32_t state = (bitmap[f / 32] & (1 << (f % 32))) ? 1 : 0;
        
        if (state == 0) total_free_frames++;
        else total_used_frames++;

        // When the state changes, print the memory block that just ended
        if (state != current_state) {
            vga_text_write(&terminal, current_state == 1 ? "[RSVD: 0x" : "[FREE: 0x");
            vga_text_write_hex(&terminal, current_run_start * 4096);
            vga_text_write(&terminal, "-0x");
            vga_text_write_hex(&terminal, ((f - 1) * 4096) + 4095);
            vga_text_write(&terminal, "] ");
            
            current_state = state;
            current_run_start = f;
        }
    }
    
    // Print the very last block of the loop
    vga_text_write(&terminal, current_state == 1 ? "[RSVD: 0x" : "[FREE: 0x");
    vga_text_write_hex(&terminal, current_run_start * 4096);
    vga_text_write(&terminal, "-0x");
    vga_text_write_hex(&terminal, ((total_frames - 1) * 4096) + 4095);
    vga_text_writeline(&terminal, "] ");

    // 2. Print the one-line summary totals
    vga_text_write(&terminal, "TOTALS -> Free: ");
    vga_text_write_dec(&terminal, total_free_frames);
    vga_text_write(&terminal, " frames (");
    vga_text_write_dec(&terminal, (total_free_frames * 4) / 1024); // KB to MB
    vga_text_write(&terminal, "MB) | Reserved: ");
    vga_text_write_dec(&terminal, total_used_frames);
    vga_text_writeline(&terminal, " frames.");
}
```

This uses a sliding window algorithm in order to print out the frames
that have the same state together, this is done so we have a good way to
debug our bitmap without taking up the whole screen.

### Functions used externally

Before we finish the PMM we just need to write our allocate and freeing
functions. Here are my implementations here:

```c
void* alloc_frame() {
    uint32_t frame_i;
    for (frame_i = 0; frame_i < total_frames; frame_i++) {
        uint32_t is_reserved = (bitmap[frame_i / 32] & (1 << (frame_i % 32)));
        if (!is_reserved) {
            bitmap_set_frame(frame_i);
            break;
        }
    } 
    return (void *)(frame_i * FRAME_SIZE);
}

void free_frame(void* frame_address) {
    uint32_t frame_i = (uint32_t)frame_address / FRAME_SIZE;
    bitmap_clear_frame(frame_i);
}
```

The freeing function just takes in the physical address of the frame and
converts it to the index in the bitmap and then clears it.
The allocating function iterates through the bitmap until a free frame
is found, and if it is, we set it and then return the physical address.
Simple! And that's the PMM all done, now we can move onto the next
stage of memory management.

