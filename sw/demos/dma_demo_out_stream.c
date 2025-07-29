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


#define N 32
#define NUM_WINDOWS 4

// #define USE_DMA

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


// int state = 64;
// void compute(uint8_t* buffer) {
//     state += 1;
//     // Fake compute function for now.
//     volatile int x = 0;
//     for (int i = 0; i < 3000; i++) {
//         x = x + 1;
//     }
//     for (int j = 0; j < N-1; j++) {
//         buffer[j] = state;
//     }
//     buffer[N-1] = '\n';
// }

#define STEPS 3
#define MAX_FORCE 100

static int16_t positions[N];
static int16_t velocities[N];
static int16_t accelerations[N];
static int initialized = 0;

static inline int16_t clamp(int16_t val, int16_t min, int16_t max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

void compute(uint8_t* buffer) {
    if (!initialized) {
        for (int i = 0; i < N; i++) {
            positions[i] = 128 + (i * 3);
            velocities[i] = 0;
        }
        initialized = 1;
    }

    for (int step = 0; step < STEPS; step++) {
        // Reset accelerations
        for (int i = 0; i < N; i++) {
            accelerations[i] = 0;
        }

        // Compute forces: every particle interacts with every other (N^2)
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                if (i == j) continue;
                int16_t dist = positions[j] - positions[i];

                // Wrap distance to range [-128,127]
                if (dist > 128) dist -= 256;
                else if (dist < -128) dist += 256;

                // Simple spring force: proportional to distance / 8
                int16_t force = dist / 8;

                // Accumulate force on particle i
                accelerations[i] += force;
            }

            // Clamp acceleration to avoid overflow
            accelerations[i] = clamp(accelerations[i], -MAX_FORCE, MAX_FORCE);
        }

        // Update velocities and positions with damping
        for (int i = 0; i < N; i++) {
            velocities[i] += accelerations[i];
            velocities[i] = (velocities[i] * 7) / 8;  // velocity damping

            positions[i] += velocities[i];

            // Wrap position to [0..255]
            if (positions[i] < 0) positions[i] += 256;
            else if (positions[i] > 255) positions[i] -= 256;
        }

        // Simple collision response: if particles too close, reverse velocities
        for (int i = 0; i < N - 1; i++) {
            for (int j = i + 1; j < N; j++) {
                int16_t diff = positions[i] - positions[j];
                if (diff > 128) diff -= 256;
                else if (diff < -128) diff += 256;

                if (diff > -4 && diff < 4) {
                    // Bounce by swapping velocities roughly
                    int16_t temp = velocities[i];
                    velocities[i] = velocities[j];
                    velocities[j] = temp;
                }
            }
        }
    }

    // Output final positions as waveform buffer
    for (int i = 0; i < N; i++) {
        buffer[i] = (uint8_t)positions[i];
    }
}


void physics_simulation() {
    int8_t buffer[N];
    
    for (int win = 0; win < NUM_WINDOWS; win++) {

        write_gpio_state(!SENDING, COMPUTING);
        compute(buffer);
        
        write_gpio_state(SENDING, !COMPUTING);
        for (int i = 0; i < N; i++) {
            uart_write(buffer[i]);
        }
        write_gpio_state(!SENDING, !COMPUTING);
    }
}


void physics_simulation_dma() {
    
    // One buffer of double length for double buffering
    // For clarity: buffer0 = buffer; buffer1 = buffer + N
    uint8_t  buffer[2*N];
    uint8_t buffer_offset = 0;     // 0 for buffer0, N for buffer1
    uint8_t* current_buffer;     // buffer + offset;
    
    uint8_t result = 0;

    for (int win = 0; win < NUM_WINDOWS; win++) {
        // Get reference to current buffer
        current_buffer = buffer + buffer_offset;
        
        write_gpio_state(dma_busy(), COMPUTING);
        compute(current_buffer);
        write_gpio_state(dma_busy(), !COMPUTING);
        
        // Wait for the dma to finish transfering previous output
        if (dma_busy()) {
            asm volatile("wfi");
        }

        write_gpio_state(!SENDING, !COMPUTING);
        enable_dma_irq();
        uart_write_dma(current_buffer, N);
        write_gpio_state(SENDING, !COMPUTING);

        // Prepare next buffer: alternate offset between 0 and N
        // if (buffer_offset == N) {buffer_offset = 0;} else {buffer_offset = N;}
        buffer_offset ^= N;
    }
    
    // Wait for the last dma transfer to finish
    if (dma_busy()) {
        asm volatile("wfi");
    }
    write_gpio_state(!SENDING, !COMPUTING);
}



int main() {
    // Setup UART
    uart_init();

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
