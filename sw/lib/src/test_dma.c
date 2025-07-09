#include "test_dma.h"

#define DMA_TEST_WORD_TRANSFER

#define DMA_TEST_INPUT_STREAM
// #define DMA_TEST_OUTPUT_STREAM
// #define DMA_TEST_DATA_TRANSFER


void* memcpy(void* dest, const void* src, unsigned len) {
    unsigned char* d = (unsigned char*) dest;
    const unsigned char* s = (const unsigned char*) src;
    while (len--) {
        *d++ = *s++;
    }
    return dest;
}

#ifdef DMA_TEST_WORD_TRANSFER
void clear_array(volatile uint32_t* arr, unsigned len)
#else
void clear_array(volatile uint8_t* arr, unsigned len)
#endif
{
    for (unsigned i = 0; i < len; i++) {
        arr[i] = 0;
    }
}

#define MAX_N 32
dma_config_t dma_config;
#ifdef DMA_TEST_WORD_TRANSFER
static volatile uint32_t src_array[MAX_N];
static volatile uint32_t dst_array[MAX_N];
#else
static volatile uint8_t src_array[MAX_N];
static volatile uint8_t dst_array[MAX_N];
#endif


#ifdef DMA_TEST_INPUT_STREAM

#ifdef DMA_TEST_WORD_TRANSFER
void print_array_hex(volatile uint32_t* arr, unsigned len)
#else
void print_array_hex(volatile uint8_t* arr, unsigned len)
#endif
{
    printf("[");
    for (unsigned i = 0; i < len-1; i++) {
        printf("%x, ", (uint8_t) arr[i]);
    }
    printf("%x]\n", (uint8_t) arr[len-1]);
}

void test_input_stream() {
    const unsigned N = 8;

    clear_array(dst_array, MAX_N);

    dma_config = (dma_config_t) {
        .src_addr = UART_BASE_ADDR,
        .dest_addr = (uint32_t)dst_array,
        .options = {
            .src_offset = UART_RBR_REG_OFFSET,
            .dst_offset = 0,
            .num_transfers = N,
            .increment_src = 0,
            .increment_dst = 1,
        #ifdef DMA_TEST_WORD_TRANSFER
            .size = DMA_TRANSFER_WORD
        #else
            .size = DMA_TRANSFER_BYTE
        #endif
        },
        .condition = {
            .cond_addr_offset = UART_LINE_STATUS_REG_OFFSET,
            .bitmask = (1 << UART_LINE_STATUS_DATA_READY_BIT),
            .conditional_type = CONDITIONAL_READ,
            .negate = 0,
            .enable = 1
        }
    };

    clear_array(dst_array, MAX_N);

    uart_read_flush();

    // Manual UART read
    uart_write(0x00);
    // sleep_ms(1);

    uint64_t start = get_mcycle();
    for (unsigned i = 0; i < N; i++) {
        dst_array[i] = uart_read();
    }
    uint64_t end = get_mcycle();

    printf("Manual UART read took %u cycles.\r\n", (uint32_t)(end - start));
    printf("Data read from UART manually: ");
    print_array_hex(dst_array, N);

    clear_array(dst_array, MAX_N);

    // DMA read
    uart_write(0x00);
    // sleep_ms(1);

    program_dma(&dma_config, 0);
    start = get_mcycle();
    activate_dma();
    while (dma_busy()) {;}
    end = get_mcycle();

    printf("DMA read took %u cycles.\r\n", (uint32_t)(end - start));
    printf("Data read stored by DMA from UART: ");
    print_array_hex(dst_array, N);
}
#else
void test_input_stream() {;}
#endif


#ifdef DMA_TEST_OUTPUT_STREAM

void test_output_stream() {
    const unsigned N = MAX_N;

    for (int i = 0; i < N; i++) {
        src_array[i] = 65 + (i % 26);
    }

    dma_config = (dma_config_t) {
        .src_addr = (uint32_t)src_array,
        .dest_addr = UART_BASE_ADDR,
        .options = {
            .src_offset = 0,
            .dst_offset = UART_RBR_REG_OFFSET,
            .num_transfers = N,
            .increment_src = 1,
            .increment_dst = 0,
        #ifdef DMA_TEST_WORD_TRANSFER
            .size = DMA_TRANSFER_WORD
        #else
            .size = DMA_TRANSFER_BYTE
        #endif
        },
        .condition = {
            .cond_addr_offset = UART_LINE_STATUS_REG_OFFSET,
            .bitmask = (1 << UART_LINE_STATUS_THR_EMPTY_BIT),
            .conditional_type = CONDITIONAL_WRITE,
            .negate = 0,
            .enable = 1
        }
    };

    // Manual UART Write
    uint64_t start = get_mcycle();
    for (unsigned i = 0; i < N; i++) {
        uart_write(src_array[i]);
    }
    uint64_t end = get_mcycle();
    putchar('\n');

    printf("Manual UART write took %u cycles.\r\n", (uint32_t)(end - start));

    // DMA write
    start = get_mcycle();
    program_dma(&dma_config, 1);
    // activate_dma();
    while (dma_busy()) {;}
    end = get_mcycle();
    putchar('\n');

    printf("DMA write took %u cycles.\r\n", (uint32_t)(end - start));
}
#else
void test_output_stream() {;}
#endif


#ifdef DMA_TEST_DATA_TRANSFER

#ifdef DMA_TEST_WORD_TRANSFER
void print_array_char(volatile uint32_t* arr, unsigned len)
#else
void print_array_char(volatile uint8_t* arr, unsigned len)
#endif
{
    putchar('[');
    for (unsigned i = 0; i < len-1; i++) {
        putchar((char) arr[i]);
        putchar(',');
        putchar(' ');
    }
    putchar((char) arr[len-1]);
    printf("]\n");
}

void test_data_transfer() {
    const unsigned N = MAX_N;
    clear_array(dst_array, MAX_N);

    for (int i = 0; i < N; i++) {
        src_array[i] = 65 + (i % 26);
    }

    dma_config = (dma_config_t) {
        .src_addr = (uint32_t)src_array,
        .dest_addr = (uint32_t)dst_array,
        .options = {
            .src_offset = 0,
            .dst_offset = 0,
            .num_transfers = N,
            .increment_src = 1,
            .increment_dst = 1,
        #ifdef DMA_TEST_WORD_TRANSFER
            .size = DMA_TRANSFER_WORD
        #else
            .size = DMA_TRANSFER_BYTE
        #endif
        },
        .condition = {
            .cond_addr_offset = 0,
            .bitmask = 0,
            .conditional_type = 0,
            .negate = 0,
            .enable = 0
        }
    };

    // Manual Data Transfer
    uint64_t start = get_mcycle();
    uint8_t len = MAX_N;
    volatile uint32_t* d = dst_array;
    volatile uint32_t* s = src_array;
    while (len--) {
        *d++ = *s++;
    }
    uint64_t end = get_mcycle();

    printf("Manual data transfer took %u cycles.\r\n", (uint32_t)(end - start));
    printf("Destination array: ");
    print_array_char(dst_array, N);
    clear_array(dst_array, MAX_N);

    // DMA Data Transfer
    start = get_mcycle();
    program_dma(&dma_config, 1);
    while (dma_busy()) {;}
    end = get_mcycle();

    printf("DMA data transfer took %u cycles.\r\n", (uint32_t)(end - start));
    printf("Destination array: ");
    print_array_char(dst_array, N);
}
#else
void test_data_transfer() {;}
#endif


void test_dma() {
    test_output_stream();
    test_input_stream();
    test_data_transfer();
}


