// Copyright (c) 2024 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0/
//
// Authors:
// - Lionel Noussi <lnoussi@ethz.ch>

#include "dma.h"
#include "uart.h"
#include "gpio.h"
#define N 32
#define NUM_WINDOWS 8

#define USE_DMA


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
        // Send start signal to testbench
        uart_write(0x00);
        
        // One buffer of double length for double buffering
        // For clarity: buffer0 = buffer; buffer1 = buffer + N
        int8_t  buffer[2*N];
        uint8_t buffer_offset = 0;     // 0 for buffer0, N for buffer1
        int8_t* current_buffer;     // buffer + offset;
        
        uint8_t result = 0;

        dma_control_struct_t dma_control_struct = {
            .src_offset = 0,
            .dst_offset = UART_RBR_REG_OFFSET,
            .num_transfers = N,
            .interrupt_enable = 1,
            .increment_src = 1,
            .increment_dst = 0,
            .transfer_size = DMA_TRANSFER_BYTE,
            .activate = 0
        };
    
        dma_condition_struct_t dma_condition_struct = {
            .cond_addr_offset = UART_LINE_STATUS_REG_OFFSET,
            .bitmask = (1 << UART_LINE_STATUS_THR_EMPTY_BIT),
            .cond_base_addr = DMA_COND_DST_BASE,
            .negate = 0,
            .enable = 1
        };
        
        dma_control_t dma_controls = encode_dma_controls(&dma_control_struct);
        program_dma((uint32_t) buffer, UART_BASE_ADDR, dma_controls, encode_dma_condition(&dma_condition_struct));

        dma_controls |= 1;  // Set activate bit in dma_controls

        for (int win = 0; win < NUM_WINDOWS; win++) {
            // Get reference to current buffer
            current_buffer = buffer + buffer_offset;
            
            gpio_write(dma_busy() << 1 | 4);
            compute(current_buffer);
            gpio_write(dma_busy() << 1);
            
            // Wait for the dma to finish transfering previous output
            if (dma_busy()) {
                asm volatile("wfi");
            }

            // Start dma to send current output
            gpio_write(0);
            enable_dma_irq();
            control_dma(dma_controls);
            gpio_write(2);

            // Prepare next buffer (alternatingly switch destination offset between 0 and N)
            buffer_offset ^= N;
            dma_controls ^= ((N & DMA_CTRL_SRC_OFFSET_MASK) << DMA_CTRL_SRC_OFFSET_SHIFT);
        }
        
        // Wait for the last dma transfer to finish
        if (dma_busy()) {
            asm volatile("wfi");
        }
        gpio_write(0);
        disable_dma_irq();  // just to be sure
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
