// Copyright (c) 2024 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0/
//
// Authors:
// - Lionel Noussi <lnoussi@ethz.ch>

#include "dma.h"
#include "uart.h"
#include "gpio.h"
#include "print.h"
#include "interrupts.h"
#include "util.h"
#include "timer.h"
#include "spi.h"

#define SENDIGN_GPIO 1
#define COMPUTING_GPIO 2

#define SENDING 1
#define COMPUTING 1

// Helper function to write different outputs and states to the gpios
uint32_t write_gpio_state(int sending, int computing) {
    gpio_write(
        (sending << SENDIGN_GPIO) |
        (computing << COMPUTING_GPIO)
    );
}


void dma_irq_handler_user() {
    gpio_toggle(2);
}


void* memcpy(void* dest, const void* src, unsigned len) {
    unsigned char* d = (unsigned char*) dest;
    const unsigned char* s = (const unsigned char*) src;
    while (len--) {
        *d++ = *s++;
    }
    return dest;
}


#define WIDTH 128
#define HEIGHT 128
#define HEIGHT_CHUNKS 32
#define H_CHUNK_SIZE (HEIGHT / HEIGHT_CHUNKS)
#define N 512
#define NUM_FRAMES 64

static const uint8_t sin_table[256] = {
    32, 34, 36, 39, 41, 43, 45, 47, 49, 51, 52, 54, 55, 56, 57, 58,
    59, 59, 60, 60, 60, 60, 60, 59, 59, 58, 57, 56, 55, 54, 52, 51,
    49, 47, 45, 43, 41, 39, 36, 34, 32, 29, 27, 25, 23, 21, 19, 17,
    15, 13, 12, 10,  9,  8,  7,  6,  5,  5,  4,  4,  4,  4,  4,  5,
     5,  6,  7,  8,  9, 10, 12, 13, 15, 17, 19, 21, 23, 25, 27, 29,
    32, 34, 36, 39, 41, 43, 45, 47, 49, 51, 52, 54, 55, 56, 57, 58,
    59, 59, 60, 60, 60, 60, 60, 59, 59, 58, 57, 56, 55, 54, 52, 51,
    49, 47, 45, 43, 41, 39, 36, 34, 32, 29, 27, 25, 23, 21, 19, 17,
    15, 13, 12, 10,  9,  8,  7,  6,  5,  5,  4,  4,  4,  4,  4,  5,
     5,  6,  7,  8,  9, 10, 12, 13, 15, 17, 19, 21, 23, 25, 27, 29,
    32, 34, 36, 39, 41, 43, 45, 47, 49, 51, 52, 54, 55, 56, 57, 58,
    59, 59, 60, 60, 60, 60, 60, 59, 59, 58, 57, 56, 55, 54, 52, 51,
    49, 47, 45, 43, 41, 39, 36, 34, 32, 29, 27, 25, 23, 21, 19, 17,
    15, 13, 12, 10,  9,  8,  7,  6,  5,  5,  4,  4,  4,  4,  4,  5,
     5,  6,  7,  8,  9, 10, 12, 13, 15, 17, 19, 21, 23, 25, 27, 29,
    32, 34, 36, 39, 41, 43, 45, 47, 49, 51, 52, 54, 55, 56, 57, 58,
};


#define RING_SPEED 4
#define MAX_DIST 256
#define CENTER_Y HEIGHT/2
#define CENTER_X WIDTH/2

void compute_next_frame(uint8_t* next_frame) {
    static uint32_t frame_index = 0;

    uint32_t full_frame_index = frame_index / HEIGHT_CHUNKS;  // div HEIGHT_CHUNKS
    uint32_t chunk_index = frame_index % HEIGHT_CHUNKS;      // mod HEIGHT_CHUNKS
    uint32_t y_start = chunk_index * H_CHUNK_SIZE;           // chunk_index * H_CHUNK_SIZE

    int ring_pos = (full_frame_index * RING_SPEED) % MAX_DIST;

    for (int y = 0; y < H_CHUNK_SIZE; y++) {
        int abs_y = y + y_start;
        int dy = abs_y - CENTER_Y;
        int y_offset = abs_y * 4;

        for (int x = 0; x < WIDTH; x++) {
            int dx = x - CENTER_X;
            int dist = dx * dx + dy * dy;

            int diff = dist - ring_pos;
            if (diff < 0) diff = -diff;

            int bg_index = (x * 4 + y_offset + full_frame_index * 4) & 0xFF;
            uint8_t bg = sin_table[bg_index];

            uint8_t intensity = (diff < 255) ? (255 - diff) >> 2 : 0;

            uint16_t pixel = bg + intensity;
            next_frame[y * WIDTH + x] = (pixel > 255) ? 255 : pixel;
        }
    }

    frame_index++;
}




void render_video_dma() {

    uint8_t frames[2][N];
    uint8_t current_frame_idx = 0;
    uint8_t* current_frame;

    for (int f = 0; f < NUM_FRAMES; f++) {
        for (int c = 0; c < HEIGHT_CHUNKS; c++) {

            current_frame = frames[current_frame_idx];
            
            write_gpio_state(dma_busy(), COMPUTING);
    
            compute_next_frame(current_frame);
    
            write_gpio_state(dma_busy(), !COMPUTING);
            
            while (dma_busy()) { asm volatile("wfi"); }
    
            write_gpio_state(!SENDING, !COMPUTING);
    
            enable_dma_irq();
            spi_write_dma(current_frame, N);
    
            write_gpio_state(SENDING, !COMPUTING);
    
            current_frame_idx = !current_frame_idx;
        }
    }
    
    // Wait for the last dma transfer to finish
    while (dma_busy()) { asm volatile("wfi"); }
    while (SPI_BUSY);
}



int main() {

    // Setup GPIO
    gpio_set_direction(0xFFFF, 0x000F); // set bottom 4 as outputs
    gpio_write(0);      // Prepare initial result
    gpio_enable(0x00FF);   // enable lowest eight

    // uint64_t start, end;
    // start = get_mcycle();
    // render_video();
    // while (SPI_BUSY);
    // end = get_mcycle();
    // printf("Keyword detection without dma took %u cycles.\n", (uint32_t) (end - start));

    // write_gpio_state(0, 0);
    // sleep_ms(8);

    // start = get_mcycle();
    render_video_dma();
    // end = get_mcycle();
    // printf("Keyword detection with dma took %u cycles.\n", (uint32_t) (end - start));
    // sleep_ms(8);
    
    return 1;
}
