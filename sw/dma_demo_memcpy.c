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

#define N 256u

void print_array(uint8_t* arr, uint16_t len) {
    uart_write('[');
    for (int i = 0; i < len - 1; i++) {
        uart_write(arr[i]);
        uart_write(',');
        uart_write(' ');
    }
    uart_write(arr[len-1]);
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


int main() {
    // Setup UART
    uart_init();

    uint64_t start, end;

    uint8_t src_arr[N];
    uint8_t dst_arr[N];

    for (int i = 0; i < N; i++) {
        src_arr[i] = (65 + (i % 26));
    }

    start = get_mcycle();
    memcpy(dst_arr, src_arr, N);
    end = get_mcycle();

    printf("Memcpy took %u cycles.\n", (uint32_t) (end - start));
    printf("Destination array is: ");
    print_array(dst_arr, N);

    // Clearing dst array
    for (int i = 0; i < N; i++) {
        dst_arr[i] = 0;
    }

    start = get_mcycle();
    memcpy_dma(dst_arr, src_arr, N);
    end = get_mcycle();

    printf("Memcpy with dma took %u cycles.\n", (uint32_t) (end - start));
    printf("Destination array is: ");
    print_array(dst_arr, N);
    uart_write_flush();
    return 1;
}
