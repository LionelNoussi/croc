// Copyright (c) 2024 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0/
//
// Authors:
// - Lionel Noussi <lnoussi@ethz.ch>

#include "dma.h"
#include "uart.h"
#include "gpio.h"

void trap_entry(void) __attribute__((interrupt));
void trap_entry(void) {
    uint32_t mcause_val;
    asm volatile ("csrr %0, mcause" : "=r"(mcause_val));
    
    if ((mcause_val & 0x80000000) && ((mcause_val & 0x1F) == 19)) {
        dma_irq_handler();  // Your function to handle + clear DMA IRQ
    }
}

#define N 32
#define NUM_WINDOWS 4

// #define USE_DMA


void* memcpy(void* dest, const void* src, unsigned len) {
    unsigned char* d = (unsigned char*) dest;
    const unsigned char* s = (const unsigned char*) src;
    while (len--) {
        *d++ = *s++;
    }
    return dest;
}


int ret_val = 0;
int compute(uint8_t* buffer) {
    // Fake compute function for now.
    volatile int x = 0;
    for (int i = 0; i < 3000; i++) {
        x = x + 1;
    }
    if (ret_val) {
        ret_val = 0;
    } else {
        ret_val = 1;
    }
    return ret_val;
}


#ifndef USE_DMA
    void keyword_detection() {
        int8_t buffer[N];
        
        int result = 0;
        
        // uart_write(0x00);
        
        for (int win = 0; win < NUM_WINDOWS; win++) {
            
            // Signal Testbench to send another input
            uart_write(0x00);
            
            gpio_write(0x2 + result);
            for (int i = 0; i < N; i++) {
                buffer[i] = uart_read();
            }
            gpio_write(0x4 + result);

            // if (win != NUM_WINDOWS -1) {
            //     uart_write(0x00);
            // }

            gpio_write(4 + result);
            result = compute(buffer);
        }
        gpio_write(result);
    }
#else
    void keyword_detection_dma() {
        // Turn GPIO 0 off
        gpio_write(2);
        
        // One buffer of double length for double buffering
        // For clarity: buffer0 = buffer; buffer1 = buffer + N
        int8_t  buffer[2*N];

        uint8_t dst_offset = 0;     // 0 for buffer0, N for buffer1
        int8_t* current_buffer;     // buffer + offset;
        
        uint8_t result = 0;

        dma_condition_struct_t dma_condition_struct = {
            .cond_addr_offset = UART_LINE_STATUS_REG_OFFSET,
            .bitmask = (1 << UART_LINE_STATUS_DATA_READY_BIT),
            .conditional_type = CONDITIONAL_READ,
            .negate = 0,
            .enable = 1
        };

        dma_control_struct_t dma_control_struct = {
            .src_offset = UART_RBR_REG_OFFSET,
            .dst_offset = 0,
            .num_transfers = N,
            .interrupt_enable = 1,
            .increment_src = 0,
            .increment_dst = 1,
            .transfer_size = DMA_TRANSFER_BYTE,
            .activate = 1
        };
        
        // Memory mapped address of dma control, so to only change controls later and not the rest
        static volatile dma_control_t* const dma_ctrl_reg = DMA_REG(DMA_CONTROL_REG_OFFSET);
        dma_control_t dma_controls = encode_dma_controls(&dma_control_struct);
        
        // Send start signal to testbench
        uart_write(0x00);
        
        // Start the DMA
        enable_dma_irq();
        program_dma(UART_BASE_ADDR, (uint32_t) buffer, dma_controls, encode_dma_condition(&dma_condition_struct));

        for (int win = 0; win < NUM_WINDOWS; win++) {
            // Write 01_ to GPIO to signal loading state
            gpio_write(2+result);   
            
            // Get reference to current buffer
            current_buffer = buffer + dst_offset;
            
            // Prepare next dma load (alternatingly switch destination offset between 0 and N)
            dst_offset ^= N;
            dma_controls ^= ((N & DMA_CTRL_DST_OFFSET_MASK) << DMA_CTRL_DST_OFFSET_SHIFT);
            
            // Wait for the dma to finish loading data into the current buffer
            if (dma_busy()) {
                asm volatile("wfi");
            }

            // Write 00_ to GPIO to signal setup state state
            gpio_write(result);   

            // Start DMA to fill next buffer, except in last iteration
            if (win != NUM_WINDOWS - 1) {
                // Write 01_ to GPIO signal loading state
                gpio_write(2+result);
                
                // Tell testbench to send another array
                uart_write(0x0);

                // Start dma again with new offset
                enable_dma_irq();
                *dma_ctrl_reg = dma_controls;
            }
            
            // Write 11_ to GPIO to indicate computing state
            gpio_write((dma_busy() << 1) + 4 + result);
            result = compute(current_buffer);
        }
        
        gpio_write(result);
        disable_dma_irq();
    }
#endif


int main() {
    // Setup interupt trap entry
    asm volatile("csrw mtvec, %0" :: "r"(trap_entry));

    // Setup UART
    uart_init();

    // Setup GPIO
    gpio_set_direction(0xFFFF, 0x000F); // lowest 3 as outputs
    gpio_write(0);      // Prepare initial result
    gpio_enable(0xF);   // enable lowest eight

    #ifndef USE_DMA
        keyword_detection();
    #else
        keyword_detection_dma();
    #endif

    compute(0);
    
    return 1;
}
