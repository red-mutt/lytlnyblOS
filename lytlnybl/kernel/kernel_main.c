#include "kernel/drivers/vga_text.h"
#include "kernel/interrupts.h"
#include "kernel/drivers/timer.h"
#include "kernel/drivers/keyboard.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "memory/heap.h"
#include "tasks/procman.h"
#include "filesystem/fs_manager.h"
#include "kernel/drivers/ata.h"
#include "tasks/loader.h"
#include "filesystem/fs.h"

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
    vga_text_writeline(&terminal, "Welcome to the lytlnyblOS!");

    idt_init();

    init_pmm();
    init_vmm();
    init_heap();
    init_procman();

    timer_init(100);
    keyboard_init();
    init_tss();
    fs_manager_init();

    vga_text_set_color(&terminal, VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    vga_text_writeline(&terminal, "Kernel multiprocessing tests:");
    process_t* test_proc= create_process(test_process, PROCESS_KERNEL);
    timer_wait_ms(10);
    vga_text_writeline(&terminal, "BACK IN MAIN PROCESS");
    vga_text_set_color(&terminal, VGA_COLOR_WHITE, VGA_COLOR_RED);


    vga_text_writeline(&terminal, "Memory allocation tests:");   
    vga_text_set_color(&terminal, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
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
    vga_text_set_color(&terminal, VGA_COLOR_WHITE, VGA_COLOR_RED);

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

    vga_text_writeline(&terminal, "Filesystem tests:");
    vga_text_set_color(&terminal, VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);

    
    fs_touch("/file.txt");

    fs_touch("/notes.txt");

    vga_text_writeline(&terminal, "Root directory contents:");
    fs_ls("/");
    vga_text_set_color(&terminal, VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    vga_text_writeline(&terminal, "Bin directory contents:");
    fs_ls("/bin");

    int32_t fd = fs_open("/notes.txt");

    const char* msg = "Hello filesystem!";
    fs_write(fd, (void*)msg, 17);
    fs_close(fd);
    fd = fs_open("/notes.txt");


    vga_text_set_color(&terminal, VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_text_writeline(&terminal, "Running shell:");
    vga_text_set_color(&terminal, VGA_COLOR_WHITE, VGA_COLOR_RED);
    load_program("/bin/shell");
    //load_program("/bin/user_test");

    for (;;);
}
