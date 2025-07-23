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


#define N 32
#define NUM_WINDOWS 8

#define USE_DMA

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


int state = 64;
void compute(uint8_t* buffer) {
    state += 1;
    // Fake compute function for now.
    volatile int x = 0;
    for (int i = 0; i < 3000; i++) {
        x = x + 1;
    }
    for (int j = 0; j < N-1; j++) {
        buffer[j] = state;
    }
    buffer[N-1] = '\n';
}


#ifndef USE_DMA
    void physics_simulation() {
        int8_t buffer[N];
        
        for (int win = 0; win < NUM_WINDOWS; win++) {

            gpio_write(4);
            compute(buffer);
            
            gpio_write(2);
            for (int i = 0; i < N; i++) {
                uart_write(buffer[i]);
            }
            gpio_write(0);
        }
    }
#else
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
#endif


int main() {
    // Setup UART
    uart_init();

    // Setup GPIO
    gpio_set_direction(0xFFFF, 0x000F); // set bottom 4 as outputs
    gpio_write(0);      // Prepare initial result
    gpio_enable(0x00FF);   // enable lowest eight

    #ifndef USE_DMA
        physics_simulation();
    #else
        physics_simulation_dma();
    #endif

    compute(0);
    
    return 1;
}
