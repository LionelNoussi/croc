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

char receive_buff[16] = {0};

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
    uint8_t N = 26;
    uint8_t NUM_WINDOWS =1;
        uint16_t addr = 0;
        int8_t buffer[N];
        
        int result = 0;
        
        for (int win = 0; win < NUM_WINDOWS; win++) {
            ssd_read_dma(buffer, addr, N);
            addr += N;

            // result = compute(buffer);
            // gpio_write(result);
        }

        for (int i = 0; i < N; i++) {
            buffer[i] = i + 210;
        }
        for (int i = 0; i < 500; i++) {
            asm volatile ("nop");
        }

        addr = 0x01A1;
        ssd_write_dma(buffer, addr, N);
        
        for (int i = 0; i < 500; i++) {
            asm volatile ("nop");
        }
        
        ssd_read_dma(buffer, addr, N);

        ssd_read_dma(buffer, addr + N, N);

        for (int i = 0; i < N; i++) {
            printf("buffer[0x%x] = 0x%x\n", i, buffer[i]);
        }

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
