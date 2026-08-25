#include "vga_text.h"
#include "interrupts.h"
#include "timer.h"
#include "keyboard.h"
#include "../memory/pmm.h"
#include "../memory/vmm.h"
#include "../memory/heap.h"
#include "../tasks/procman.h"
#include "../filesystem/fs.h"
#include "../filesystem/fs_manager.h"

#include <stdint.h>

vga_text terminal;
#include "../kernel/mappings.h"

void test_process() {
    vga_text_writeline(&terminal, "NEW KERNEL PROCESS RUNNING");
    for (;;);
}

void kernel_main(void)
{
    volatile char* vga = (volatile char*)0xB8000;
    
    //signal that we have reached C
    vga[0] = 'C';
    vga[1] = 0x02;

    vga_text_init(&terminal);
    vga_text_writeline(&terminal, "Welcome to the lytlnybl kernel in protected mode");

    idt_init();

    init_pmm();
    init_vmm();
    init_heap();
    init_procman();

    timer_init(100);
    keyboard_init();
    init_tss();

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

    process_t* test_proc= create_process(test_process, PROCESS_KERNEL);
    timer_wait_ms(10);

    vga_text_writeline(&terminal, "back in main");
    
    //userspace testing

    void* code_frame = alloc_frame();

    extern unsigned char _binary_user_test_bin_start[];
    extern unsigned char _binary_user_test_bin_end[];

    //copy user program to physical frame
    uintptr_t user_size = (uintptr_t)(_binary_user_test_bin_end - 
            _binary_user_test_bin_start);

    for (uintptr_t i = 0; i < user_size; i++) {
        ((uint8_t*)code_frame)[i] = _binary_user_test_bin_start[i];
    }

    //context switch is never called a third time, what?
    process_t* user_proc = create_process((void*)USER_CODE_BASE, PROCESS_USER);

    map_page(
        user_proc->page_directory,
        USER_CODE_BASE, 
        (uintptr_t)code_frame, 
        PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER
    );

    //map vga so process ring 3 can access
    map_page(
        user_proc->page_directory,
        USER_VGA,
        0xB8000,
        PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER
    );


    
    init_ata();

    uint8_t test_write[512];
    uint8_t test_read[512];

    for (int i = 0; i < 512; i++)
        test_write[i] = 0xAA;

    if (!ata_write_sector(50, (void*)test_write)) {
        vga_text_writeline(&terminal, "ATA WRITE FAIL");
    }

    if (!ata_read_sector(50, (void*)test_read)) {
        vga_text_writeline(&terminal, "ATA READ FAIL");
    }

    for (int i = 0; i < 512; i++) {
        if (test_read[i] != test_write[i]) {
            vga_text_write_dec(&terminal, i);
            vga_text_writeline(&terminal, " ATA DATA FAILLLLLLLLLLLLLLLLLLLLLLLLLLL");
            break;
        }
    }
    // FILE SYSTEM TESTS
    
    if (!fs_format())
      vga_text_writeline(&terminal, "failed formating");

    /*
    fs_inode_t root;

    if (!fs_read_inode(FS_ROOT_INODE, &root)) {
      vga_text_writeline(&terminal, "failed to get root inode");
    }

    //create file 

    int32_t notes = fs_create_file(FS_ROOT_INODE, "notes.txt");
    if (notes < 0)
      vga_text_writeline(&terminal, "failed to create notes.txt");

    // write to file

    const char* msg = "Hello filesystem!";
    if (fs_write_file(notes, msg, 17, 0) != 17)
      vga_text_writeline(&terminal, "failed to write to notes.txt");

    //read full text
    
    char buf[18];
    if (fs_read_file(notes, buf, 17, 0) != 17) 
      vga_text_writeline(&terminal, "failed to read file");
    buf[17] = '\0';

    vga_text_write(&terminal, "notes.txt: ");
    vga_text_writeline(&terminal, buf);

    //read partial text
    char partial[6];
    if (fs_read_file(notes, partial, 5, 6) != 5)
        vga_text_writeline(&terminal, "partial read failed");
    partial[5] = '\0';

    vga_text_write(&terminal, "partial: ");
    vga_text_writeline(&terminal, partial);

    //create directory
    int32_t docs = fs_create_directory(FS_ROOT_INODE, "docs");

    if (docs < 0)
        vga_text_writeline(&terminal, "directory failed");

    // create file inside of directory and write to it
    int32_t inside = fs_create_file(docs, "hello.txt");

    if (inside < 0)
        vga_text_writeline(&terminal, "nested file failed");

    const char* hello = "hello";

    fs_write_file(inside, hello, 5, 0);

    //read file in directory
    char hello_buf[6];

    fs_read_file(inside, hello_buf, 5, 0);

    hello_buf[5] = '\0';

    vga_text_write(&terminal, "nested: ");
    vga_text_writeline(&terminal, hello_buf);

    vga_text_write_dec(&terminal, fs_resolve_path("/docs/hello.txt"));
    */

    fs_mkdir("/dir1");
    fs_mkdir("/dir2");
    fs_touch("file.txt");

    fs_ls("/");
    fs_touch("/dir1/notes.txt");
    fs_ls("/dir1");
    fs_rm("/dir2");
    fs_ls("/");

    int32_t fd = fs_open("/dir1/notes.txt");

    const char* msg = "Hello filesystem!";
    fs_write(fd, (void*)msg, 17);
    fs_close(fd);
    fd = fs_open("/dir1/notes.txt");
    char recv[18];
    fs_read(fd, (void*)recv, 17);  

    recv[17] = '\0';
    vga_text_writeline(&terminal, recv);


    for (;;);
}
