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
#define THRESHOLD 40000

#define USE_DMA

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


// 8-bit fixed-point cosine LUT for 32 points (scaled by 127)
const int8_t cos_lut[32] = {
    127, 125, 117, 106, 90, 71, 49, 25,
    0, -25, -49, -71, -90, -106, -117, -125,
    -127, -125, -117, -106, -90, -71, -49, -25,
    0, 25, 49, 71, 90, 106, 117, 125
};


static inline int16_t mul8x8(int8_t a, int8_t b) {
    uint16_t ua = (a < 0) ? -a : a;
    uint8_t ub = (b < 0) ? -b : b;

    int16_t result = 0;
    while (ub) {
        if (ub & 1)
            result += ua;
        ua <<= 1;
        ub >>= 1;
    }

    if ((a ^ b) < 0)  // sign bit differs -> negative result
        result = -result;

    return result;
}

int detect_keyword(uint8_t* time_window) {
    int8_t* samples = (int8_t*)time_window;

    const unsigned quarter_N = N / 4;  // 8 for N=32
    const unsigned half_N = N / 2;

    // Loop over frequency bins k
    for (unsigned k = 0; k <= half_N; ++k) {
        int32_t real_sum = 0;
        int32_t imag_sum = 0;
        uint8_t phase = 0;  // phase accumulator for (k * n) mod N

        // Inner loop: accumulate DFT real and imaginary parts
        for (unsigned n = 0; n < N; ++n) {
            uint8_t cos_index = phase;                      // cos phase index
            uint8_t sin_index = (phase + quarter_N) & 31;  // sin phase index (90° shift). Module N=32 by doing &31

            int8_t sample = samples[n];

            int16_t real_part = mul8x8(sample, cos_lut[cos_index]);
            int16_t imag_part = mul8x8(sample, cos_lut[sin_index]);

            real_sum += real_part;
            imag_sum += imag_part;

            phase = (phase + k) & 31;  // increment phase modulo N
        }

        // Approximate magnitude using sum of absolute values (cheap abs)
        int32_t abs_real = (real_sum < 0) ? -real_sum : real_sum;
        int32_t abs_imag = (imag_sum < 0) ? -imag_sum : imag_sum;
        int32_t magnitude_approx = abs_real + abs_imag;

        // Threshold check for keyword detection
        if (magnitude_approx > THRESHOLD) {
            return 1;  // keyword detected
        }
    }

    return 0;  // no keyword detected
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
            result = detect_keyword(buffer);
        }
        gpio_write(result);
    }
#else
    void keyword_detection_dma() {
        // Send start signal to testbench
        uart_write(0x00);
        
        // One buffer of double length for double buffering
        // For clarity: buffer0 = buffer; buffer1 = buffer + N
        int8_t  buffer[2*N];
        uint8_t dst_offset = 0;     // 0 for buffer0, N for buffer1
        int8_t* current_buffer = buffer;     // buffer + offset;
        
        uint8_t result = 0;

        // Start the DMA
        write_gpio_state(LOADING, !COMPUTING, 0);
        uart_read_dma(current_buffer, N);

        for (int win = 0; win < NUM_WINDOWS; win++) {
            
            // Get reference to current buffer
            // if (dst_offset == N) {dst_offset = 0;} else {dst_offset = N;}
            current_buffer = buffer + dst_offset;
            
            // Wait for the dma to finish loading data into the current buffer
            if (dma_busy()) {
                asm volatile("wfi");
            }

            write_gpio_state(!LOADING, !COMPUTING, result);  

            // Start DMA to fill next buffer, except in last iteration
            if (win != NUM_WINDOWS - 1) {
                write_gpio_state(LOADING, COMPUTING, result);
                
                // Alternate offset between 0 and N
                dst_offset ^= N;
                // Tell testbench to send another array
                uart_write(0x0);
                // Start dma
                uart_read_dma(buffer + dst_offset, N);
            }

            result = detect_keyword(current_buffer);

            write_gpio_state(dma_busy(), !COMPUTING, result);
        }
        
        write_gpio_state(!LOADING, !COMPUTING, result);
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
        keyword_detection();
    #else
        keyword_detection_dma();
    #endif
    
    return 1;
}