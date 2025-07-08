#include "dma.h"
#include "util.h"
#include "config.h"
#include "print.h"
#include "timer.h"
#include "uart.h"


int dma_busy() {
    return *reg32(DMA_BASE_ADDR, DMA_CONTROL_REG_OFFSET) & 0x1;
}

int dma_ready() {
    return !dma_busy();
}

void *memset(void *s, int c, unsigned long n) {
    unsigned char *p = s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

void dma_test() {
    uint64_t start, end;
    uint8_t offset = UART_RBR_REG_OFFSET;
    uint32_t repeat = 8;
    uint8_t byte_mode = 1;
    uint8_t activate = 1;
    uint32_t control = ((uint32_t)offset << 24) |
                  ((repeat & 0x7FF) << 13) |
                  ((uint32_t)(byte_mode & 0x1) << 1) |
                  (activate & 0x1);
    uint32_t condition = (
        (uint32_t) UART_LINE_STATUS_REG_OFFSET << 24 |
        (uint32_t) (1 << UART_LINE_STATUS_DATA_READY_BIT) << 16 |
        (uint32_t) ((uint32_t) 1 & 0x3)
    );

    // volatile uint8_t test_source[2] = {0x1A, 1};    // Value to copy, condition
    volatile uint8_t test_array[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    // printf("Source address: %x \r\n", test_source);
    // printf("Destiantion address: %x \r\n", test_array);

    volatile uint32_t* dma_src = reg32(DMA_BASE_ADDR, DMA_SRC_REG_OFFSET);
    volatile uint32_t* dma_tgt = reg32(DMA_BASE_ADDR, DMA_TGT_REG_OFFSET);
    volatile uint32_t* dma_ctr = reg32(DMA_BASE_ADDR, DMA_CONTROL_REG_OFFSET);
    volatile uint32_t* dma_cond = reg32(DMA_BASE_ADDR, DMA_CONDITION_REG_OFFSET);
    volatile uint32_t* dma_interrupt = reg32(DMA_BASE_ADDR, DMA_INTERRUPT_OFFSET);

    // Start the transmission, by sending the start byte
    uart_read_flush();
    uart_write((uint8_t) 0xAA);
    sleep_ms(50);

    start = get_mcycle();
    // for (int i = 0; i < 8; i++) {
    //     test_array[i] = uart_read();
    // }
    test_array[0] = uart_read();
    test_array[1] = uart_read();
    test_array[2] = uart_read();
    test_array[3] = uart_read();
    test_array[4] = uart_read();
    test_array[5] = uart_read();
    test_array[6] = uart_read();
    test_array[7] = uart_read();
    end = get_mcycle();

    printf("Reading manually takes %u cycles.\r\n", (uint32_t) (end - start));
    printf("Data read from UART manually: [");
    for (int i = 0; i < 7; i++) {
        printf("%x, ", test_array[i]);
    }
    printf("%x]\n", test_array[7]);

    for (int i = 0; i < 8; i++) {
        test_array[i] = 0;
    }
    uart_read_flush();
    uart_write((uint8_t) 0xAA);
    sleep_ms(50);

    start = get_mcycle();
    *dma_src = (uint32_t) UART_BASE_ADDR;
    *dma_tgt = (uint32_t) &test_array[0];
    *dma_cond = condition;
    *dma_ctr = control;
    // uint32_t controls_on = *dma_ctr;
    // uint32_t src_addr = *dma_src;
    // uint32_t tgt_addr = *dma_tgt;
    // uint32_t read_condition = *dma_cond;

    while (dma_busy()) {;}

    // uint32_t controls_off = *dma_ctr;
    end = get_mcycle();

    printf("Required cycles by DMA: %u \r\n", (uint32_t) (end - start));
    // printf("Written source addr: %x \r\n", src_addr);
    // printf("Written target addr: %x \r\n", tgt_addr);
    // printf("Written condition registers: %b \r\n", read_condition);
    // printf("Written control registers: %b \r\n", controls_on);
    // printf("Read control registers: %b \r\n", controls_off);

    printf("Data read stored by DMA from UART: [");
    for (int i = 0; i < 7; i++) {
        printf("%x, ", test_array[i]);
    }
    printf("%x]\n", test_array[7]);
}


void uart_dma_receive(void *dst, uint32_t len) {
    // 1. Write destination address to dma
    // 2. Write UART base address to dma
    // 3. Write the correct condition, which waits for UART to be ready, to dma
    // In one write:
        // Set the corect offset to read a byte
        // Set the repeat amount to len
        // Set range/offset-select to offset
        // Set 8/32bit-select to 8-bit
        // Activate dma
}

void uart_dma_transmit(void *src, uint32_t len) {
    
}