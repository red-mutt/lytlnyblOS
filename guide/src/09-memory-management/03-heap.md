# Part XI : Kernel Heap

## What are we doing?

To put it short, the point of this section is to regain back the
features provided by malloc and free in C that we lost not having an
operating system that provides these features (until now). If you
weren\'t aware we need access to malloc because currently our memory is
static, every size is known at compile time, but for a more functional
OS we need to make things where size or lifetime is unknown. A good
example of where we need this is in filesystems, where the sizes of
files and directories are unknown. Another thing to know is that the
versions of malloc and free we are writing are for use only inside of
the kernel, if we choose to run code outside of the kernel in userspace,
we will need different versions.

For this allocator, we are not actually writing a driver for any sort of
hardware, this is completely software. You may be thinking of the Memory
managment unit (MMU), but this is a device for translating and
protecting memory addresses. So we actually interfaced with this in the
previous chapter and writing the paging could have been considered
writing a driver.

### How does the heap work?

In computing there are two meanings of the word heap, one is a data
structure which satisfies the \"heap condition\" and is not related to
the one we are creating which is heap memory which is simply a region of
memory used for dynamic memory allocation

Somewhere in memory, we can allocate a page for our heap. Initially the
allocator does not know anything about which blocks are free and such.
So we use a small ammount of the heap space to make a header. This
contains info such as the size of the current block, whether it is free
and the (virtual) address of the next block.

Speaking of blocks, the heap is made up of them, initially it is one big
block when all memory is free. Every block has it\'s own header and due
to the next member, the collection of headers form a linked list, and
then this is essentially the structure of our heap. And then malloc will
traverse the linked list of headers looking for the next free block and
whether it\'s big enough and then will return the pointer if successful.
free will just take the address and then set the memory to free. But
there is a problem with fragmentation here, where after enough use we
may have 300 bytes free, but no segment that is a size of 250 bytes due
to having many smaller segments that are all seperate. This is fixed
using coalescing, where we merge adjacent free blocks. This all seems
simple enough.

We must also remember that we are not creating the heap on raw memory,
we are building it on top of paging, this is the first thing in our
kernel which will directly utilize the VMM and PMM, so we will have to
allocate pages depending on the required size of the heap.

## The code

### The header

This is our blueprint for the heap:

    #ifndef HEAP_H
    #define HEAP_H

    #define HEAP_START 0x00400000 //dir index 1, eveyrthing else 0

    #include 
    #include 
    #include 

    typedef struct heap_header{
        size_t size;
        bool free;
        struct heap_header* next;
    } heap_header_t;

    void init_heap(void);

    void* kmalloc(size_t requested_size);
    void kfree(void* ptr);

    void expand_heap(size_t required_size);

    // helpers

    heap_header_t* find_free_block(size_t requested_size);

    void split_block(
        heap_header_t* block,
        size_t size
    );

    void merge_blocks(
        heap_header_t* block
    );

    heap_header_t* get_header(void* ptr);


    #endif

The first thing we define is a virtual address for the start of the
heap, this is at page table 1, page 0, address 0. We then make a type
for the heap header that has everything we discussed prior. We then
define init_heap, and then malloc and free. We then have a function that
will expand the pages used by the heap if it\'s too full. And then we
have our helpers for finding free blocks, splitting blocks, merging
blocks and getting a header from a virtual address.

### Implementation

    #include "heap.h"
    #include "vmm.h"
    #include "pmm.h"

    heap_header_t* heap_start_head;
    uintptr_t heap_end;

    void init_heap() {
        void* physical_start = alloc_frame();
        map_page(HEAP_START, (uintptr_t)physical_start, PAGE_PRESENT | PAGE_WRITABLE);
        heap_header_t* first_header = (heap_header_t*)HEAP_START;

        first_header->size = 4096 - sizeof(heap_header_t); //4096 is page size 
        first_header->free = true;
        first_header->next = NULL;

        heap_start_head = first_header;
        heap_end = HEAP_START + 0x1000;
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

    void split_block(heap_header_t* block, size_t requested_size) {
        size_t space_remaining = (block->size - requested_size);
        if (space_remaining < sizeof(heap_header_t) + sizeof(uint8_t)) {
            return;
            //don't make block, not big enough
        };

        uintptr_t origin_block_end = (uintptr_t)(block) + block->size + sizeof(heap_header_t);
        uintptr_t new_block_start = (uintptr_t)(block) + requested_size + sizeof(heap_header_t);
        heap_header_t* new_block = (heap_header_t*)(new_block_start);
        
        new_block->size = origin_block_end - new_block_start - sizeof(heap_header_t);
        new_block->free = true;

        heap_header_t* temp_next = block->next;
        new_block->next = temp_next;
        block->next = new_block;
        
        block->size = requested_size;
    }

    void expand_heap(size_t required_size) {
        uint32_t required_pages = (required_size + 4096 - 1) / 4096;
        for (size_t i = 0; i < required_pages; i++) {
            void* new_page_physical_start = alloc_frame();
            map_page(heap_end, (uintptr_t)new_page_physical_start, PAGE_PRESENT | PAGE_WRITABLE);

            heap_header_t* traversal_head = heap_start_head;
            while (traversal_head) {
                if (!traversal_head->next && !traversal_head->free) {
                    heap_header_t* new_frame_header = (heap_header_t*)heap_end;
            
                    new_frame_header->free = true;
                    new_frame_header->next = NULL;
                    new_frame_header->size = 4096 - sizeof(heap_header_t);

                    traversal_head->next = new_frame_header;
                    break;
                } else if (!traversal_head->next && traversal_head->free) {
                    traversal_head->size += 4096;
                    break; //will just be empty space for the previous free head        
                }
                traversal_head = traversal_head->next;
            }
            heap_end += 0x1000;
        } 
    }

    void* kmalloc(size_t requested_size) {
        if (!requested_size) return NULL;

        heap_header_t* block = find_free_block(requested_size);
        if (block) {
            split_block(block, requested_size);
            block->free = false;
            return (void*)((uintptr_t)block + sizeof(heap_header_t)); //return free memory not the head
        } else {
            expand_heap(requested_size);
            return kmalloc(requested_size);
        }
    }

    heap_header_t* get_header(void* ptr) {
        return (heap_header_t*)((uintptr_t)ptr - sizeof(heap_header_t));
    }

    void merge_blocks(heap_header_t* block) {
        if (!block->next || !block->next->free) return;
        heap_header_t* block_to_merge = block->next;
        
        block->size += block_to_merge->size + sizeof(heap_header_t);
        block->next = block_to_merge->next;
        return merge_blocks(block); //do to next
    }

    void kfree(void* ptr) {
        if (!ptr) return;
        heap_header_t* block_to_free = get_header(ptr);
        block_to_free->free = true;
        merge_blocks(block_to_free);
    }

Our heap has two global variables that are used in order to track the
heap. The first is the start head, this is what we look at whenever we
want to traverse the list, as the heap is just one big linked list. The
next is the heap end, which just contains the final virtual address of
the heap, this is used for extending the heap.

### init_heap

To start, we allocate a frame using the PMM, Then we map this to the
virtual address `HEAP_START` with present and writable flags, then we
map map a pointer to a heap_header to the virtual address we just
allocated.\
This is our typical workflow for using our implementation of paging from
now on. After we allocate memory we fill our first header with the
applicable data then we set this to the global variable and then set
heap start to the end of this page.

### find_free_block

This function takes in a requested_size in bytes. and then it traverses
the linked list. if it\'s free and the size is larger than the requested
size, then we return it, if nothing is found we return null.

### split_block

So when we have a block that\'s big enough, we don\'t want to use a 30
byte block to allocate 5 bytes of data. So we must split it. The way we
split it is by making a smaller block with the required size, and then a
second which is just the extra space.

First in our function we store the space remaining after we allocate the
block of the size we want. If the remaining space isn\'t big enough to
contain a header and at least a byte then we don\'t split it and just
return. First we get the start for the big block that was the origin,
next we get the start of the block we create for the free space that\'s
not required. We then make a new block at the location of the pointer.
Then we set the size of the new block to be the empty space after the
required and set it to free, then we change the \"next\" variables of
both of the headers. Finally, we set the size of the original block
(which is now the block that contains the required space) to the size of
the required space.

### expand_heap

First we calculate required pages and then iterate a loop for each page
to make where we allocate a frame, map it to a page. Next we then
traverse the head until we get to the final head in the linked list. If
the block is free we just increse it\'s size to extend to the next page.
If the final block is not free, we have to make a head at the start of
the page with the appropriate data. After traversal we then move the
`heap_end` to the new end of the heap.

### kmalloc

Now it\'s time to put together everything we\'ve made before in order to
allocate memory. First we find a free block to use, if a block isn\'t
found, we expand the heap by the requested size that was passed to
kmalloc and then we recursively call `kmalloc` again. If there is a
block, we use the split function with the requested size, set the block
to not be free, and then return the pointer for the DATA and not the
whole block.

### merge_blocks

When freeing memory we need to use coalescing, this is why we make the
function to merge blocks. This function takes a block and if the next
block is null or if the next block isn\'t free, it instantly returns.\
If the function can execute we increase the size of the first block to
span over the next block we are merging to it We then change the
original block\'s next to skip over the next block. We don\'t need to
set the data to 0 as it is marked as free anyway. After merging we
recursively call `merge_blocks` which will stop recusion if we find a
used block or we are at the end.

### kfree

Just a simple function, we convert the pointer to a heap header using
the function, we just set the block to free, and then call merge blocks
on that block.

## Testing

Testing is pretty simple, we can do it all within our main function,
seen here:

    #include "vga_text.h"
    #include "interrupts.h"
    #include "timer.h"
    #include "keyboard.h"
    #include "../memory/pmm.h"
    #include "../memory/vmm.h"
    #include "../memory/heap.h"

    #include 

    vga_text terminal;

    void kernel_main(void)
    {
        volatile char* vga = (volatile char*)0xB8000;
        
        //signal that we have reached C
        vga[0] = 'C';
        vga[1] = 0x02;

        vga_text_init(&terminal);
        vga_text_writeline(&terminal, "Welcome to the lytlnybl kernel in protected mode");

        idt_init();
        timer_init(100);
        keyboard_init();
        init_pmm(); 
        init_vmm();
        init_heap();

        uint32_t* numbers = (uint32_t*)kmalloc(5 * sizeof(uint32_t));

        for (int i = 0; i < 5; i++) {
            numbers[i] = (i + 1) * 10; // Stores 10, 20, 30, 40, 50
        }

        vga_text_write(&terminal, "Values: ");
        for (int i = 0; i < 5; i++) {
            vga_text_write_dec(&terminal, numbers[i]);
            vga_text_write(&terminal, " ");
        }
        vga_text_writeline(&terminal, "");

        kfree(numbers);

        for (;;);
    }

Just like in regular C code, Nice! Now memory managment is fully made
which is a pretty big milestone.

