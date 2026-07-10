#include "pmm.h"
#include "../kernel/vga_text.h"

extern vga_text terminal;

uint32_t* bitmap = (uint32_t*)BITMAP_BASE;
uint32_t total_frames = 0;
uint32_t bitmap_entries = 0;

void bitmap_set_frame(uint32_t frame_index) {
    bitmap[frame_index / 32] |= (1 << (frame_index % 32));
}
void bitmap_clear_frame(uint32_t frame_index) {
    bitmap[frame_index / 32] &= ~(1 << (frame_index % 32)); 
}

void init_pmm() {
    uint16_t map_entry_count = *(uint16_t*)0x4FFC;
    memory_map_entry_t* memory_map = (memory_map_entry_t*)0x5000;

    /*
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
    */

    uint64_t max_usable_address = 0;

    // find highest usable physical RAM address to calc max frames
    for (uint16_t i = 0; i < map_entry_count; i++) {
        uint64_t highest_access = memory_map[i].base + memory_map[i].length;
        if (highest_access > max_usable_address) max_usable_address = highest_access;
    }

    // calculate bitmap dimensions
    total_frames = max_usable_address / FRAME_SIZE;
    bitmap_entries = (total_frames + 8 - 1) / 8; //always round up
                                                    //
    // set all regions to reserved for safety
    for (uint32_t i = 0; i < bitmap_entries; i++) {
        bitmap[i] = 0xFFFFFFFF;
    }
    
}
