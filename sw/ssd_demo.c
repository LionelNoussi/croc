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
#include "timer.h"
#include "interrupts.h"
#include "spi.h"

#define N 8
#define NUM_WINDOWS 2
#define THRESHOLD 40000

// #define USE_DMA

#define OUTPUT_GPIO 0
#define LOADING_GPIO 1
#define COMPUTING_GPIO 2

#define LOADING 1
#define COMPUTING 1


// Helper function to write different outputs and states to the gpios
uint32_t write_gpio_state(int loading, int computing, int output) {
    gpio_write(
        (loading << LOADING_GPIO) |
        (computing << COMPUTING_GPIO) |
        (output << OUTPUT_GPIO)
    );
}

// Override of weak function defined in interrupts.h
void dma_irq_handler_user() {
    gpio_toggle(1 << LOADING_GPIO);
}


void print_array(uint8_t* array, uint8_t len) {
    uart_write('[');
    for (int i = 0; i < len - 1; i++) {
        uart_write(array[i]);
        uart_write(',');
        uart_write(' ');
    }
    uart_write(array[len-1]);
    uart_write(']');
    uart_write('\n');
}


void* memcpy(void* dst, const void* src, unsigned num_bytes) {
    unsigned len;
    if ((num_bytes & 3) == 0 && (((uint32_t) src & 3) == 0) && (((uint32_t) dst & 3) == 0)) {
        len = num_bytes >> 2;
        uint32_t* dw = (uint32_t*) dst;
        const uint32_t* sw = (const uint32_t*) src;
        for (int i = 0; i < len; i++) {
            dw[i] = sw[i];
        }
    } else {
        len = num_bytes;
        unsigned char* d = (unsigned char*) dst;
        const unsigned char* s = (const unsigned char*) src;
        for (int i = 0; i < len; i++) {
            d[i] = s[i];
        }
    }
    return dst;
}


uint8_t ret_val = 1;
uint8_t compute(uint8_t* buffer) {
    for (int i = 0; i < 500; i++) {
        asm volatile ("nop");
    }
    ret_val = !ret_val;
    return ret_val;
}


#ifndef USE_DMA
    void normal_example() {
        uint16_t addr = 65;
        uint8_t buffer[N];
        for (int i = 0; i < N; i++) {
            buffer[i] = 1;
        }
        
        int result = 0;
        
        for (int win = 0; win < NUM_WINDOWS; win++) {
            write_gpio_state(LOADING, !COMPUTING, result);
            spi_read_full(addr, buffer, N);
            // ssd_read_dma(buffer, addr, N);
            addr += N;
            printf("Read array: ");
            print_array(buffer, N);
        }

        // addr = 0x01A1;
        // ssd_write_dma(buffer, addr, N);
        
        // for (int i = 0; i < 500; i++) {
        //     asm volatile ("nop");
        // }
        
        // ssd_read_dma(buffer, addr, N);
    }
#else
    void dma_examble() {
        uint16_t addr = 0;
        
        // One buffer of double length for double buffering
        // For clarity: buffer0 = buffer; buffer1 = buffer + N
        int8_t  buffer[2*N];
        uint8_t dst_offset = 0;     // 0 for buffer0, N for buffer1
        int8_t* current_buffer = buffer;     // buffer + offset;
        
        uint8_t result = 0;

        // Start the DMA
        write_gpio_state(LOADING, !COMPUTING, 0);
        ssd_read_dma(current_buffer, addr, N);

        for (int win = 0; win < NUM_WINDOWS; win++) {
            // Get reference to current buffer
            // if (dst_offset == N) {dst_offset = 0;} else {dst_offset = N;}
            current_buffer = buffer + dst_offset;
            
            // Wait for the dma to finish loading data into the current buffer
            if (dma_busy()) {
                asm volatile("wfi");
            }

            write_gpio_state(!LOADING, COMPUTING, result);  

            // Start DMA to fill next buffer, except in last iteration
            if (win != NUM_WINDOWS - 1) {
                write_gpio_state(LOADING, COMPUTING, result);
                
                // Alternate offset between 0 and N
                dst_offset ^= N;
                // Start dma
                addr += N;
                ssd_read_dma(buffer + offset, addr, N);
            }

            result = compute(current_buffer);

            write_gpio_state(dma_busy(), !COMPUTING, result);
        }
    }
#endif


int main() {
    // Setup UART
    uart_init();

    // Setup GPIO
    gpio_set_direction(0xFFFF, 0x000F); // lowest 3 as outputs
    gpio_write(0);      // Prepare initial result
    gpio_enable(0xF);   // enable lowest eight

    #ifndef USE_DMA
    normal_example();
    #else
    dma_example();
    #endif
    
    uart_write('\n');
    sleep_ms(1);
    return 1;
}
