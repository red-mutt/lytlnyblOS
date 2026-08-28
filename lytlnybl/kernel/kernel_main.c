#include "kernel/drivers/vga_text.h"
#include "kernel/interrupts.h"
#include "kernel/drivers/timer.h"
#include "kernel/drivers/keyboard.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "memory/heap.h"
#include "tasks/procman.h"
#include "filesystem/fs.h"
#include "filesystem/fs_manager.h"
#include "kernel/drivers/ata.h"
#include "kernel/mappings.h"
#include "tasks/loader.h"

#include <stdint.h>

vga_text terminal;

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

    /*
    void* code_frame = alloc_frame();

    extern unsigned char _binary_user_test_bin_start[];
    extern unsigned char _binary_user_test_bin_end[];

    //copy user program to physical frame
    uintptr_t user_size = (uintptr_t)(_binary_user_test_bin_end - 
            _binary_user_test_bin_start);

    for (uintptr_t i = 0; i < user_size; i++) {
        ((uint8_t*)code_frame)[i] = _binary_user_test_bin_start[i];
    }

    process_t* user_proc = create_process((void*)USER_CODE_BASE, PROCESS_USER);

    map_page(
        user_proc->page_directory,
        USER_CODE_BASE, 
        (uintptr_t)code_frame, 
        PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER
    );
    */

    /*
    //map vga so process ring 3 can access
    map_page(
        user_proc->page_directory,
        USER_VGA,
        0xB8000,
        PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER
    );
    */

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

    init_ata();
    fs_mkdir("/dir1");
    fs_mkdir("/dir2");
    fs_touch("/file.txt");

    fs_ls("/");
    fs_touch("/dir1/notes.txt");
    fs_ls("/dir1");
    fs_rm("/dir2");
    fs_ls("/");
    fs_ls("/bin");

    int32_t fd = fs_open("/dir1/notes.txt");

    const char* msg = "Hello filesystem!";
    fs_write(fd, (void*)msg, 17);
    fs_close(fd);
    fd = fs_open("/dir1/notes.txt");
    char recv[18];
    fs_read(fd, (void*)recv, 17);  


    recv[17] = '\0';
    vga_text_writeline(&terminal, recv);

    load_program("/bin/shell");
    load_program("/bin/user_test");

    for (;;);
}
