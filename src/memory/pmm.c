#include "pmm.h"
#include "../kernel/vga_text.h"

extern vga_text terminal;

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
}
