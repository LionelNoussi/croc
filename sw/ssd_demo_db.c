// Copyright 2023 ETH Zurich and University of Bologna.
// Solderpad Hardware License, Version 0.51, see LICENSE for details.
// SPDX-License-Identifier: SHL-0.51

// Lars Kröger <lkroeger@ethz.ch>

#include "uart.h"
#include "print.h"
#include "timer.h"
#include "gpio.h"
#include "util.h"
#include "dma.h"
#include "spi.h"


static volatile uint32_t* const dma_src_reg         = DMA_REG(DMA_SRC_REG_OFFSET);
static volatile uint32_t* const dma_dst_reg         = DMA_REG(DMA_TGT_REG_OFFSET);
static volatile uint32_t* const dma_ctrl_reg        = DMA_REG(DMA_CONTROL_REG_OFFSET);
static volatile uint32_t* const dma_cond_reg        = DMA_REG(DMA_CONDITION_REG_OFFSET);
static volatile uint32_t* const dma_activate_reg    = DMA_REG(DMA_ACTIVATE_OFFSET);
static volatile uint32_t* const dma_irq_reg         = DMA_REG(DMA_INTERRUPT_OFFSET);
static volatile uint32_t* const dma_status_reg      = DMA_REG(DMA_STATUS_OFFSET);


void ssd_read_dma(uint8_t* destination_array, uint16_t addr, uint16_t num_bytes) {
    while (dma_busy());
    while(SPI_BUSY == 0x01);   // in case the SPI is still finishing while the DMA already returned during write
    spi_empty_rx();

    uint8_t status;
    const uint8_t addr_hi = (addr >> 8) & 0xFF;  // upper 8 bits
    const uint8_t addr_lo = addr & 0xFF;
    const uint8_t control_on = (0 << 7) | (1 << 1) | 0x1;
    const uint8_t control_rst = (0 << 1) | 0x0;

    *dma_src_reg = (uint32_t) SPI_BASE_ADDR;
    *dma_dst_reg = (uint32_t) destination_array;
    *dma_cond_reg = (uint32_t) (
        (SPI_FIFOSTAT_OFFSET            << DMA_COND_OFFSET_SHIFT)       |
        (SPI_STATUS_RX_EMPTY_MASK       << DMA_COND_MASK_SHIFT)         |
        (DMA_COND_SRC_BASE              << DMA_COND_BASE_ADDR_SHIFT)    |
        (1                              << DMA_COND_NEGATE_SHIFT)       |
        (1                              << DMA_COND_ENABLE_SHIFT)
    );
    const dma_control_t dma_controls = (uint32_t) (
        (SPI_RX_REG_OFFSET  << DMA_CTRL_SRC_OFFSET_SHIFT)       |
        (0                  << DMA_CTRL_DST_OFFSET_SHIFT)       |
        (1                  << DMA_CTRL_IRQ_ENABLE_SHIFT)       |
        (0                  << DMA_CTRL_INC_SRC_SHIFT)          |
        (1                  << DMA_CTRL_INC_DEST_SHIFT)         |
        (DMA_TRANSFER_BYTE  << DMA_CTRL_TRANSFER_SIZE_SHIFT)    |
        (1                  << DMA_CTRL_ACTIVATE_SHIFT)
    ) | ((num_bytes & DMA_CTRL_NUM_TRANSFERS_MASK) << DMA_CTRL_NUM_TRANSFERS_SHIFT);

    // Pre-fill SPI TX Buffer with correct bytes to start SSD protocol without pause
    SPI_TX = control_on;
    SPI_TX = num_bytes;
    SPI_TX = addr_hi;
    SPI_TX = addr_lo;
    SPI_MODE_CTRL = 0b00000000;
    SPI_LENGTH = num_bytes + 3;
    SPI_FREQ = 0x05;
    SPI_CTRL = control_on;
    // Load four first dummy responses
    for (uint8_t i = 0; i < 4; i++) {

        // Stall while, RX buffer is empty
        while (SPI_FIFOSTAT & SPI_STATUS_RX_EMPTY_MASK);

        SPI_RX;
    }

    *dma_ctrl_reg = dma_controls;
    if (dma_busy()) {
        asm volatile ("wfi");
    }
}


void ssd_write_dma(uint8_t* source_array, uint16_t addr, uint16_t num_bytes) {
    while (dma_busy());
    while(SPI_BUSY); // in case the SPI is still finishing while the DMA already returned during write
    uint8_t status, dummy;
    const uint8_t addr_hi = (addr >> 8) & 0xFF; 
    const uint8_t addr_lo = addr & 0xFF;
    const uint8_t control_on = (0 << 3) | (2 << 1) | 0x1;
    const uint8_t control_rst = (0 << 3) | (2 << 1) | 0x0;  

    *dma_src_reg = (uint32_t) source_array;
    *dma_dst_reg = (uint32_t) SPI_BASE_ADDR;
    *dma_cond_reg = (uint32_t) (
        (SPI_FIFOSTAT_OFFSET            << DMA_COND_OFFSET_SHIFT)       |
        (SPI_STATUS_TX_ALMOST_FULL_MASK << DMA_COND_MASK_SHIFT)         |
        (DMA_COND_DST_BASE              << DMA_COND_BASE_ADDR_SHIFT)    |
        (0                              << DMA_COND_NEGATE_SHIFT)       |
        (1                              << DMA_COND_ENABLE_SHIFT)
    );
    const dma_control_t dma_controls = (uint32_t) (
        (0                  << DMA_CTRL_SRC_OFFSET_SHIFT)       |
        (SPI_TX_REG_OFFSET  << DMA_CTRL_DST_OFFSET_SHIFT)       |
        (1                  << DMA_CTRL_IRQ_ENABLE_SHIFT)       |
        (1                  << DMA_CTRL_INC_SRC_SHIFT)          |
        (0                  << DMA_CTRL_INC_DEST_SHIFT)         |
        (DMA_TRANSFER_BYTE  << DMA_CTRL_TRANSFER_SIZE_SHIFT)    |
        (1                  << DMA_CTRL_ACTIVATE_SHIFT)
    ) | ((num_bytes & DMA_CTRL_NUM_TRANSFERS_MASK) << DMA_CTRL_NUM_TRANSFERS_SHIFT);

    // Pre-fill SPI TX Buffer with correct bytes to start SSD protocol without pause
    SPI_TX = control_on;
    SPI_TX = num_bytes;
    SPI_TX = addr_hi;
    SPI_TX = addr_lo;

    // Telling the SPI how many transactions it should do
    // This is done to facilitate unbroken communication
    // If the TX buffer is empty at any point, it will continue to send 0x0
    SPI_LENGTH = num_bytes + 3;
    SPI_FREQ = 0x5;
    SPI_CTRL = control_on;
    // Turning dma on to stream in rest of the data.
    *dma_ctrl_reg = dma_controls;

}

void ssd_demo(){

    uint16_t N = 124;
    uint8_t NUM_WINDOWS =10;
    int8_t buffer[N];
    uint8_t address = 0x32;

    for(uint16_t i =0; i<N; i++){
        buffer[i] = i +2;
    }

    // for (int win = 0; win < NUM_WINDOWS; win++) {
    //     enable_dma_irq();
    //     ssd_write_dma(buffer,address,N);
    //     enable_dma_irq();
    //     ssd_read_dma(buffer,address,N);
    //     while(dma_busy());
    //     address += N;
    // }
    // ssd_read_dma(buffer, address,N);
    // while(dma_busy());

    buffer[0] = 0x4;
    buffer[1] = N;
    uint16_t clkdiv = 0x05;
    spi_init(SPI_MODE_0, clkdiv);
    spi_write(buffer,N);
}

int main() {
    uart_init(); // setup the uart peripheral
    ssd_demo();
    while(dma_busy());
    printf("Done with SPI test\n");
    uart_write_flush();
    return 1;
}



