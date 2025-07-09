#include "test_dma.h"


// void *memset(void *s, int c, unsigned long n) {
//     unsigned char *p = s;
//     while (n--) {
//         *p++ = (unsigned char)c;
//     }
//     return s;
// }


void print_array_hex(volatile uint8_t* arr, unsigned len) {
    printf("[");
    for (unsigned i = 0; i < len-1; i++) {
        printf("%x, ", arr[i]);
    }
    printf("%x]\n", arr[len-1]);
}

void clear_array(volatile uint8_t* arr, unsigned len) {
    for (unsigned i = 0; i < len; i++) {
        arr[i] = 0;
    }
}

void test_dma() {
    const unsigned N = 8;
    volatile uint8_t test_array[N];

    dma_config_t dma_config = {
        .src_addr = UART_BASE_ADDR,
        .dest_addr = (uint32_t)test_array,
        .options = {
            .src_offset = UART_RBR_REG_OFFSET,
            .num_transfers = N,
            .increment_source = 0,
            .increment_dest = 1,
            .size = DMA_TRANSFER_BYTE
        },
        .condition = {
            .cond_addr_offset = UART_LINE_STATUS_REG_OFFSET,
            .bitmask = (1 << UART_LINE_STATUS_DATA_READY_BIT),
            .negate = 0,
            .enable = 1
        }
    };

    clear_array(test_array, N);

    // Manual UART read
    uart_write(0xAA);
    sleep_ms(1);

    uint64_t start = get_mcycle();
    for (unsigned i = 0; i < N; i++) {
        test_array[i] = uart_read();
    }
    uint64_t end = get_mcycle();

    printf("Manual UART read took %u cycles.\r\n", (uint32_t)(end - start));
    printf("Data read from UART manually: ");
    print_array_hex(test_array, N);

    clear_array(test_array, N);

    // DMA read
    uart_write(0xAA);
    sleep_ms(1);

    program_dma(&dma_config, 0);
    start = get_mcycle();
    activate_dma();
    while (dma_busy()) {;}
    end = get_mcycle();

    printf("DMA read took %u cycles.\r\n", (uint32_t)(end - start));
    printf("Data read stored by DMA from UART: ");
    print_array_hex(test_array, N);
}
