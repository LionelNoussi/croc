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

    // DMA TEST
    // test_dma();

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
    uint8_t tx_data[10] = {1,2,3,4,5,6,7,8,9,10};  // example data

    


    uint8_t rx_data[10] = {0};
    rx_data[0] = 1;
    for(uint8_t i = 0; i < 10; i++){
        rx_data[i] = i + 1;
    }
    uint16_t addr = 0x0034;
    spi_write_full(addr, rx_data, 10);
    addr = 0x00a4;
    spi_read_full(addr,rx_data,10);
    for(uint8_t i = 0; i< 10; i++){
        printf("received spi data value:0x%x \n", rx_data[i]);
    }

    addr = 0x0034;
    spi_read_full(addr, rx_data,10);


    // printf("SPI read from 0x%x: 0x%x\n", addr, result_spi);
    printf("Done with SPI test\n");
    // printf("SPI returned: 0x%x\n", result_spi);
    printf("Done with Spi test \n");
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
