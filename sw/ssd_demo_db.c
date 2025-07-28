// Copyright (c) 2024 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0/
//
// Authors:
// - Philippe Sauter <phsauter@iis.ee.ethz.ch>

#include "uart.h"
#include "print.h"
#include "timer.h"
#include "gpio.h"
#include "util.h"
#include "dma.h"

#include "spi.h"

#define USE_DMA
#define OUTPUT_GPIO 0
#define LOADING_GPIO 1
#define COMPUTING_GPIO 2


// Helper function to write different outputs and states to the gpios
uint32_t write_gpio_state(int loading, int computing, int output) {
    gpio_write(
        (loading << LOADING_GPIO) |
        (computing << COMPUTING_GPIO) |
        (output << OUTPUT_GPIO)
    );
}

/// @brief Example integer square root
/// @return integer square root of n
uint32_t isqrt(uint32_t n) {
    uint32_t res = 0;
    uint32_t bit = (uint32_t)1 << 30;

    while (bit > n) bit >>= 2;

    while (bit) {
        if (n >= res + bit) {
            n -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }
    return res;
}

static inline void delay_cycles(volatile uint32_t cycles) {
    while (cycles--) {
        __asm__ volatile ("nop");
    }
}

char receive_buff[16] = {0};

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


// int detect_keyword(uint8_t* time_window) {
//     int8_t* samples = (int8_t*)time_window;

//     const unsigned quarter_N = N / 4;  // 8 for N=32
//     const unsigned half_N = N / 2;

//     // Loop over frequency bins k
//     for (unsigned k = 0; k <= half_N; ++k) {
//         int32_t real_sum = 0;
//         int32_t imag_sum = 0;
//         uint8_t phase = 0;  // phase accumulator for (k * n) mod N

//         // Inner loop: accumulate DFT real and imaginary parts
//         for (unsigned n = 0; n < N; ++n) {
//             uint8_t cos_index = phase;                      // cos phase index
//             uint8_t sin_index = (phase + quarter_N) & 31;  // sin phase index (90° shift). Module N=32 by doing &31

//             int8_t sample = samples[n];

//             int16_t real_part = mul8x8(sample, cos_lut[cos_index]);
//             int16_t imag_part = mul8x8(sample, cos_lut[sin_index]);

//             real_sum += real_part;
//             imag_sum += imag_part;

//             phase = (phase + k) & 31;  // increment phase modulo N
//         }

//         // Approximate magnitude using sum of absolute values (cheap abs)
//         int32_t abs_real = (real_sum < 0) ? -real_sum : real_sum;
//         int32_t abs_imag = (imag_sum < 0) ? -imag_sum : imag_sum;
//         int32_t magnitude_approx = abs_real + abs_imag;

//         // Threshold check for keyword detection
//         if (magnitude_approx > THRESHOLD) {
//             return 1;  // keyword detected
//         }
//     }

//     return 0;  // no keyword detected
// }

void compute(int8_t* buffer){
    delay_cycles(250);
}

void ssd_demo(){

#ifdef USE_DMA
    uint8_t N = 200;
    uint8_t NUM_WINDOWS =4;
    int8_t buffer1[N];
    int8_t buffer2[N];
    uint8_t address = 0;

    for (int win = 0; win < NUM_WINDOWS; win++) {
        while(dma_busy());
        enable_dma_irq();
        if((win  %2) == 0){
             ssd_read_dma(buffer1, address, N);
             write_gpio_state(1,0,0);
             compute(buffer2);
             write_gpio_state(0,0,0);
        }
        else {
             ssd_read_dma(buffer2, address, N);
             write_gpio_state(1,0,0);
             compute(buffer1);
             write_gpio_state(0,0,0);
        }
        address += N;
        // for (int i = 0; i < 1; i++) {
        //     printf("buffer[0x%x] = 0x%x\n", i, buffer1[i]);
        // }
    }
#else
            uint8_t N = 50;
    uint8_t NUM_WINDOWS =1;
    uint16_t addr = 0;
    int8_t buffer1[N];
    int8_t buffer2[N];
        
        int result = 0;
        
        for (int win = 0; win < NUM_WINDOWS; win++) {
            ssd_read_dma(buffer1, addr, N);
            addr += N;

            // result = compute(buffer);
            // gpio_write(result);
        }
        int16_t mult;
        for (int i = 0; i < N; i++) {
            mult = mul8x8(buffer2[N],10);
        }
        // for (int i = 0; i < 500; i++) {
        //     asm volatile ("nop");
        // }

        addr = 0x01A1;
        enable_dma_irq();
        ssd_write_dma(buffer1, addr, N);
        
        for (int i = 0; i < 500; i++) {
            asm volatile ("nop");
        }
        enable_dma_irq();
        ssd_read_dma(buffer1, addr, N);
        enable_dma_irq();
        ssd_read_dma(buffer1, addr + N, N);

        delay_cycles(10000);

        for (int i = 0; i < N; i++) {
            printf("buffer[0x%x] = 0x%x\n", i, buffer1[i]);
        }
#endif

    
}

int main() {
    uart_init(); // setup the uart peripheral

    // simple printf support (only prints text and hex numbers)
    printf("Hello World!\n");
    // wait until uart has finished sending
    uart_write_flush();

    // ROM TEST -------------------------------------
    // Read from the Rom and print the result
    uint32_t val;
    printf("ROM content: ");
    // Reading 12 characters. Should be "LN&LK's ASIC"
    for (int i = 0; i < 12; i += 4) {
        val = *reg32(USER_ROM_BASE_ADDR, i);    // Reads 4 chars from ROM at once
        printf((char*) &val);     // Cast to char array and printf
    }
    uart_write('\n');
    // ROM TEST END ---------------------------------

    // uart loopback
    uart_loopback_enable();
    printf("internal msg\n");
    sleep_ms(1);
    for(uint8_t idx = 0; idx<15; idx++) {
        receive_buff[idx] = uart_read();
        if(receive_buff[idx] == '\n') {
            break;
        }
    }
    uart_loopback_disable();

    printf("Loopback received: ");
    printf(receive_buff);
    uart_write_flush();

    // toggling some GPIOs
    gpio_set_direction(0xFFFF, 0x000F); // lowest four as outputs
    gpio_write(0x0A);  // ready output pattern
    gpio_enable(0xFF); // enable lowest eight
    // wait a few cycles to give GPIO signal time to propagate
    asm volatile ("nop; nop; nop; nop; nop;");
    printf("GPIO (expect 0xA0): 0x%x\n", gpio_read());

    gpio_toggle(0x0F); // toggle lower 8 GPIOs
    asm volatile ("nop; nop; nop; nop; nop;");
    printf("GPIO (expect 0x50): 0x%x\n", gpio_read());
    uart_write_flush();

    printf("Starting spi test \n");
    uart_write_flush();
    ssd_demo();

    while(dma_busy());
    printf("Done with SPI test\n");

    // printf("SPI returned: 0x%x\n", result_spi);
    uart_write_flush();

    // doing some computes
    uint32_t start = get_mcycle();
    uint32_t res   = isqrt(1234567890UL);
    uint32_t end   = get_mcycle();
    printf("Result: 0x%x, Cycles: 0x%x\n", res, end - start);
    uart_write_flush();

    // using the timer
    printf("Tick\n");
    sleep_ms(10);
    printf("Tock\n");
    uart_write_flush();
    return 1;
}



