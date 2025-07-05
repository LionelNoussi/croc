#include "dma.h"
#include "util.h"
#include "config.h"
#include "print.h"
#include "timer.h"
#include "uart.h"


void dma_test() {
    uint8_t offset = 8;
    uint32_t repeat = 11;
    uint8_t byte_or_word = 0;
    uint8_t activate = 1;
    uint32_t control = ((uint32_t)offset << 24) |
                  ((repeat & 0x7FF) << 13) |
                  ((uint32_t)(byte_or_word & 0x1) << 1) |
                  (activate & 0x1);
    uint32_t condition = (
        (uint32_t) 10 << 24 |
        (uint32_t) 20 << 16 |
        (uint32_t) ((uint32_t) 3 & 0x3)
    );

    volatile uint32_t* dma_src = reg32(DMA_BASE_ADDR, DMA_SRC_REG_OFFSET);
    volatile uint32_t* dma_tgt = reg32(DMA_BASE_ADDR, DMA_TGT_REG_OFFSET);
    volatile uint32_t* dma_ctr = reg32(DMA_BASE_ADDR, DMA_CONTROL_REG_OFFSET);
    volatile uint32_t* dma_cond = reg32(DMA_BASE_ADDR, DMA_CONDITION_REG_OFFSET);
    volatile uint32_t* dma_interrupt = reg32(DMA_BASE_ADDR, DMA_INTERRUPT_OFFSET);

    uint64_t start = get_mcycle();
    *dma_src = 1;
    *dma_tgt = 2;
    *dma_cond = control;
    *dma_ctr = control;
    uint32_t src_addr = *dma_src;
    uint32_t tgt_addr = *dma_tgt;
    uint32_t controls_on = *dma_ctr;
    *dma_interrupt = DMA_INTERRUPT_SIGNAL;
    uint32_t controls_off = *dma_ctr;
    uint32_t read_condition = *dma_cond;
    uint64_t end = get_mcycle();

    printf("Required cycles: %u \r\n", (uint32_t) (end - start));
    printf("Written source addr: %x \r\n", src_addr);
    printf("Written target addr: %x \r\n", tgt_addr);
    printf("Written condition registers: %b \r\n", read_condition);
    printf("Written control registers: %b \r\n", controls_on);
    printf("Written control registers: %b \r\n", controls_off);

    uart_write_flush();
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