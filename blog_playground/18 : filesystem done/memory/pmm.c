#include "memory/pmm.h"
#include "kernel/drivers/vga_text.h"

extern vga_text terminal;

uint32_t* bitmap = (uint32_t*)BITMAP_BASE;
uint32_t total_frames = 0;
uint32_t bitmap_entries = 0;
extern char _kernel_start;
extern char _kernel_end;

void bitmap_set_frame(uint32_t frame_index) {
    bitmap[frame_index / 32] |= (1 << (frame_index % 32));
}
void bitmap_clear_frame(uint32_t frame_index) {
    bitmap[frame_index / 32] &= ~(1 << (frame_index % 32)); 
}

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


    //print_bitmap_summary();

}

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
