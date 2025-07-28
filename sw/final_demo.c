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


void monitor_write_dma(uint8_t* buffer, uint16_t num_bytes);
void monitor_write(uint8_t* buffer, uint16_t num_bytes);


#define WIDTH 16
#define HEIGHT 8
#define N (WIDTH * HEIGHT)
#define NUM_FRAMES 4

void compute_next_frame(int8_t* next_frame) {
    static int x = 0;
    static int y = HEIGHT - 2;
    static int vx = 1;
    static int vy = -1;
    static int gravity = 1;

    // Clear frame
    for (int i = 0; i < N; i++) next_frame[i] = 0;

    // Update position
    x += vx;
    y += vy;
    vy += gravity;

    // Bounce on floor
    if (y <= 0) {
        y = -y;
        vy = -((vy * 3) >> 2); // dampened bounce
    }

    if (x < 0) x = 0;
    if (x >= WIDTH) x = WIDTH - 1;
    if (y < 0) y = 0;
    if (y >= HEIGHT) y = HEIGHT - 1;

    // Wrap horizontally
    if (x >= WIDTH) x = 0;
    if (x < 0) x = WIDTH - 1;

    next_frame[y * WIDTH + x] = 1;
}


void render_video() {
    int8_t frame[N];
    
    for (int f = 0; f < NUM_FRAMES; f++) {

        write_gpio_state(!SENDING, COMPUTING);

        compute_next_frame(frame);
        
        write_gpio_state(SENDING, !COMPUTING);

        monitor_write(frame, N);

        write_gpio_state(!SENDING, !COMPUTING);
    }
}


void render_video_dma() {
    write_gpio_state(!SENDING, !COMPUTING);

    uint8_t frames[2][N];
    uint8_t current_frame_idx = 0;
    uint8_t* current_frame;

    for (int f = 0; f < NUM_FRAMES; f++) {

        uint8_t* current_frame = frames[current_frame_idx];
        
        write_gpio_state(dma_busy(), COMPUTING);

        compute(current_frame);

        write_gpio_state(dma_busy(), !COMPUTING);
        
        while (dma_busy()) { asm volatile("wfi"); }

        write_gpio_state(!SENDING, !COMPUTING);

        enable_dma_irq();
        monitor_write_dma(current_frame, N);

        write_gpio_state(SENDING, !COMPUTING);

        current_frame_idx = !current_frame_idx;
    }
    
    // Wait for the last dma transfer to finish
    while (dma_busy()) { asm volatile("wfi"); }

    write_gpio_state(!SENDING, !COMPUTING);
}



int main() {

    // Setup GPIO
    gpio_set_direction(0xFFFF, 0x000F); // set bottom 4 as outputs
    gpio_write(0);      // Prepare initial result
    gpio_enable(0x00FF);   // enable lowest eight

    uint64_t start, end;
    start = get_mcycle();
    physics_simulation();
    end = get_mcycle();
    // printf("Keyword detection without dma took %u cycles.\n", (uint32_t) (end - start));

    write_gpio_state(0, 0);
    sleep_ms(8);

    start = get_mcycle();
    physics_simulation_dma();
    end = get_mcycle();
    // printf("Keyword detection with dma took %u cycles.\n", (uint32_t) (end - start));
    
    return 1;
}
