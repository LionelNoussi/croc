// Copyright (c) 2024 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0/
//
// Authors:
// - Lionel Noussi <lnoussi@ethz.ch>

#include "dma.h"
#include "uart.h"
#include "util.h"
#include "print.h"

#define N 64

void* memcpy(void* dest, const void* src, unsigned len) {
    unsigned char* d = (unsigned char*) dest;
    const unsigned char* s = (const unsigned char*) src;
    // while (len--) {
    //     *d++ = *s++;
    // }
    for (int i = 0; i < len; i++) {
        d[i] = s[i];
    }
    return dest;
}


void* memcpy_dma(void* dest, const void* src, unsigned len) {
    dma_control_t controls = 0x1F | (len << DMA_CTRL_NUM_TRANSFERS_SHIFT);
    enable_dma_irq();
    program_dma((uint32_t) src, (uint32_t) dest, controls, 0);
    asm volatile ("wfi");
    return dest;
}


int main() {
    // Setup UART
    uart_init();

    uint64_t start, end;

    uint8_t src_arr[N];
    uint8_t dst_arr[N];

    for (int i = 0; i < N; i++) {
        src_arr[i] = i;
    }

    start = get_mcycle();
    memcpy(dst_arr, src_arr, N);
    end = get_mcycle();

    printf("Memcpy took %u cycles.\n", (uint32_t) (end - start));

    // Clearing dst array
    for (int i = 0; i < N; i++) {
        dst_arr[i] = 0;
    }

    start = get_mcycle();
    memcpy_dma(dst_arr, src_arr, N);
    end = get_mcycle();

    printf("Memcpy with dma took %u cycles.\n", (uint32_t) (end - start));
    uart_write_flush();
    return 1;
}
