#include "dma.h"
#include "util.h"
#include "config.h"
#include "print.h"
#include "timer.h"
#include "uart.h"


static volatile uint32_t* const dma_src_reg         = DMA_REG(DMA_SRC_REG_OFFSET);
static volatile uint32_t* const dma_dst_reg         = DMA_REG(DMA_TGT_REG_OFFSET);
static volatile uint32_t* const dma_ctrl_reg        = DMA_REG(DMA_CONTROL_REG_OFFSET);
static volatile uint32_t* const dma_cond_reg        = DMA_REG(DMA_CONDITION_REG_OFFSET);
static volatile uint32_t* const dma_activate_reg    = DMA_REG(DMA_ACTIVATE_OFFSET);
static volatile uint32_t* const dma_irq_reg         = DMA_REG(DMA_INTERRUPT_OFFSET);
static volatile uint32_t* const dma_status_reg      = DMA_REG(DMA_STATUS_OFFSET);


// Encodes DMA control word from control struct
dma_control_t encode_dma_controls(const dma_control_struct_t* opts) {
    return ((opts->src_offset        & DMA_CTRL_SRC_OFFSET_MASK)      << DMA_CTRL_SRC_OFFSET_SHIFT)     |
           ((opts->dst_offset        & DMA_CTRL_DST_OFFSET_MASK)      << DMA_CTRL_DST_OFFSET_SHIFT)     |
           ((opts->num_transfers     & DMA_CTRL_NUM_TRANSFERS_MASK)   << DMA_CTRL_NUM_TRANSFERS_SHIFT)  |
           ((opts->interrupt_enable  & DMA_CTRL_IRQ_ENABLE_MASK)      << DMA_CTRL_IRQ_ENABLE_SHIFT)     |
           ((opts->increment_src     & DMA_CTRL_INC_MASK)             << DMA_CTRL_INC_SRC_SHIFT)        |
           ((opts->increment_dst     & DMA_CTRL_INC_MASK)             << DMA_CTRL_INC_DEST_SHIFT)       |
           ((opts->transfer_size     & DMA_CTRL_TRANSFER_SIZE_MASK)   << DMA_CTRL_TRANSFER_SIZE_SHIFT)  |
           ((opts->activate          & DMA_CTRL_ACTIVATE_MASK)        << DMA_CTRL_ACTIVATE_SHIFT);
}


// Encodes DMA condition word from condition struct
dma_condition_t encode_dma_condition(const dma_condition_struct_t* cond) {
    return ((cond->cond_addr_offset  & DMA_COND_OFFSET_MASK)          << DMA_COND_OFFSET_SHIFT)         |
           ((cond->bitmask           & DMA_COND_MASK_MASK)            << DMA_COND_MASK_SHIFT)           |
           ((cond->cond_base_addr    & DMA_COND_BASE_ADDR_MASK)       << DMA_COND_BASE_ADDR_SHIFT)      |
           ((cond->negate            & DMA_COND_NEGATE_MASK)          << DMA_COND_NEGATE_SHIFT)         |
           ((cond->enable            & DMA_COND_ENABLE_MASK)          << DMA_COND_ENABLE_SHIFT);
}


void program_dma(uint32_t src_addr, uint32_t dst_addr, dma_control_t controls, dma_condition_t condition) {
    *dma_src_reg    = src_addr;
    *dma_dst_reg    = dst_addr;
    *dma_cond_reg   = condition;
    *dma_ctrl_reg   = controls;
}

void activate_dma() {
    *dma_activate_reg = 1;
}

void interrupt_dma() {
    do {
        *dma_irq_reg = 1;
    } while (dma_busy());
}

dma_status_t read_dma_status() {
    uint32_t raw_status = *dma_status_reg;

    dma_status_t status = {
        .completed_receives =      (raw_status >> DMA_STATUS_COMPLETED_RCVS_SHIFT) & DMA_STATUS_COMPLETED_RCVS_MASK -1,
        .completed_transmissions = (raw_status >> DMA_STATUS_COMPLETED_TRMS_SHIFT) & DMA_STATUS_COMPLETED_TRMS_MASK -1,
        .active =                  (raw_status >> DMA_STATUS_ACTIVE_SHIFT) & DMA_STATUS_ACTIVE_MASK
    };

    return status;
}

int dma_busy() {
    return (*dma_status_reg >> DMA_STATUS_ACTIVE_SHIFT) & DMA_STATUS_ACTIVE_MASK;
}

void enable_dma_irq(void) {
    // Enable DMA fast interrupt bit 3
    asm volatile("csrs mie, %0" ::"r"(MIE_DMA_IRQ_BIT));
    // Enable global interrupts
    asm volatile("csrsi mstatus, 8" ::: "memory");
}

void disable_dma_irq(void) {
    asm volatile("csrc mie, %0" ::"r"(MIE_DMA_IRQ_BIT));
}


void uart_read_dma(uint8_t* destination_array, uint8_t N) {
    enable_dma_irq();
    *dma_src_reg = UART_BASE_ADDR;
    *dma_dst_reg = (uint32_t) destination_array;
    *dma_cond_reg = 0x14010001;
    *dma_ctrl_reg = 0x17 | ((N & DMA_CTRL_NUM_TRANSFERS_MASK) << DMA_CTRL_NUM_TRANSFERS_SHIFT);

    /*
    Magic numbers of condition and control reg come from:

    // Condition is static and is always the same struct
    dma_condition_struct_t dma_condition_struct = {
        .cond_addr_offset = UART_LINE_STATUS_REG_OFFSET,
        .bitmask = (1 << UART_LINE_STATUS_DATA_READY_BIT),
        .cond_base_addr = DMA_COND_SRC_BASE,
        .negate = 0,
        .enable = 1
    };
    encode_dma_condition(&dma_condition_struct) --> 0x14010001;

    // Only variable part is the number of transfers N
    dma_control_struct_t dma_control_struct = {
        .src_offset = UART_RBR_REG_OFFSET,
        .dst_offset = 0,
        .num_transfers = 0, // replace with N later
        .interrupt_enable = 1,
        .increment_src = 0,
        .increment_dst = 1,
        .transfer_size = DMA_TRANSFER_BYTE,
        .activate = 1
    };
    encode_dma_controls(&dma_control_struct) --> 0x17;
    */
}

void uart_write_dma(uint8_t* source_array, uint8_t N) {
    enable_dma_irq();
    *dma_src_reg = (uint32_t) source_array;
    *dma_dst_reg = UART_BASE_ADDR;
    *dma_cond_reg = 0x14200005;
    *dma_ctrl_reg = 0x1B | ((N & DMA_CTRL_NUM_TRANSFERS_MASK) << DMA_CTRL_NUM_TRANSFERS_SHIFT);

    /*
    Magic numbers of condition and control reg come from:

    // Condition is static and is always the same struct
    dma_condition_struct_t dma_condition_struct = {
        .cond_addr_offset = UART_LINE_STATUS_REG_OFFSET,
        .bitmask = (1 << UART_LINE_STATUS_THR_EMPTY_BIT),
        .cond_base_addr = DMA_COND_DST_BASE,
        .negate = 0,
        .enable = 1
    };
    encode_dma_condition(&dma_condition_struct) --> 0x14200005;

    // Only variable part is the number of transfers N
    dma_control_struct_t dma_control_struct = {
        .src_offset = 0,
        .dst_offset = UART_THR_REG_OFFSET,
        .num_transfers = 0,
        .interrupt_enable = 1,
        .increment_src = 1,
        .increment_dst = 0,
        .transfer_size = DMA_TRANSFER_BYTE,
        .activate = 1
    };

    encode_dma_controls(&dma_control_struct) --> 0x1B;
    */
}


void* memcpy_dma(void* dst, const void* src, unsigned num_bytes) {
    dma_control_t controls;
    enable_dma_irq();
    if ((num_bytes & 3) == 0 && (((uint32_t) src & 3) == 0) && (((uint32_t) dst & 3) == 0)) {
        unsigned num_words = num_bytes >> 2;
        controls = 0x1D | ((num_words & DMA_CTRL_NUM_TRANSFERS_MASK) << DMA_CTRL_NUM_TRANSFERS_SHIFT);
    } else {
        controls = 0x1F | ((num_bytes & DMA_CTRL_NUM_TRANSFERS_MASK) << DMA_CTRL_NUM_TRANSFERS_SHIFT);
    }
    *dma_src_reg = (uint32_t) src;
    *dma_dst_reg = (uint32_t) dst;
    *dma_cond_reg = 0;
    *dma_ctrl_reg = controls;
    asm volatile ("wfi");
    return dst;

    /*
    // Only variable part is the number of transfers N
    dma_control_struct_t dma_control_struct = {
        .src_offset = 0,
        .dst_offset = 0,
        .num_transfers = 0, // Change later
        .interrupt_enable = 1,
        .increment_src = 1,
        .increment_dst = 1,
        .transfer_size = DMA_TRANSFER_BYTE or DMA_TRANSFER_WORD,
        .activate = 1
    };

    encode_dma_controls(&dma_control_struct) --> 0x1F if byte 0x1D if word;
    */
}


#include "spi.h"
#define SPI_STATUS_RX_EMPTY_MASK 32
#define SPI_FIFOSTAT_OFFSET 0x1C
#define SPI_STATUS_TX_ALMOST_FULL_MASK 0x3
#define SPI_TX_REG_OFFSET 0x8
#define SPI_RX_REG_OFFSET 0xC

void ssd_read_dma(uint8_t* destination_array, uint16_t addr, uint8_t num_bytes) {
    while (dma_busy());
    uint8_t status;
    const uint8_t addr_hi = (addr >> 8) & 0xFF;  // upper 8 bits
    const uint8_t addr_lo = addr & 0xFF;
    const uint8_t control_on = (5 << 3) | (1 << 1) | 0x1;
    const uint8_t control_rst = (5 << 3) | (1 << 1) | 0x0;

    *dma_src_reg = (uint32_t) SPI_BASE_ADDR;
    *dma_dst_reg = (uint32_t) destination_array;
    *dma_cond_reg = (uint32_t) (
        (SPI_FIFOSTAT_OFFSET            << DMA_COND_OFFSET_SHIFT)       |
        (SPI_STATUS_RX_EMPTY_MASK       << DMA_COND_MASK_SHIFT)         |
        (DMA_COND_DST_BASE              << DMA_COND_BASE_ADDR_SHIFT)    |
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

    // Telling the SPI how many transactions it should do
    // This is done to facilitate unbroken communication
    // If the TX buffer is empty at any point, it will continue to send 0x0
    SPI_LENGTH = num_bytes + 3;
    SPI_CTRL = control_on;
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    SPI_CTRL = control_rst;
    
    // Load four first dummy responses
    for (uint8_t i = 0; i < 4; i++) {

        // Stall while, RX buffer is empty
        while (SPI_FIFOSTAT & SPI_STATUS_RX_EMPTY_MASK);

        SPI_RX;
    }

    // *dma_ctrl_reg = dma_controls;
    for (uint8_t i = 0; i < num_bytes; i++) {
        // Stall while, RX buffer is empty
        while (SPI_FIFOSTAT & SPI_STATUS_RX_EMPTY_MASK);

        // Store result in destination array
        destination_array[i] = SPI_RX;
    }

    // This blocks now, because the condition is probably wrong,
    // and so the dma never reads enough bytes. If this works,
    // remove it from the function, since the user is supposed to call it.
    // if (dma_busy()) {
    //     asm volatile ("wfi");
    // }
}


void ssd_write_dma(uint8_t* source_array, uint16_t addr, uint8_t num_bytes) {
    while (dma_busy());

    uint8_t status, dummy;
    const uint8_t addr_hi = (addr >> 8) & 0xFF;  // upper 8 bits
    const uint8_t addr_lo = addr & 0xFF;
    const uint8_t control_on = (5 << 3) | (2 << 1) | 0x1;
    const uint8_t control_rst = (5 << 3) | (2 << 1) | 0x0;

    *dma_src_reg = (uint32_t) source_array;
    *dma_dst_reg = (uint32_t) SPI_BASE_ADDR;
    *dma_cond_reg = (uint32_t) (
        (SPI_FIFOSTAT_OFFSET            << DMA_COND_OFFSET_SHIFT)       |
        (SPI_STATUS_TX_ALMOST_FULL_MASK << DMA_COND_MASK_SHIFT)         |
        (DMA_COND_DST_BASE              << DMA_COND_BASE_ADDR_SHIFT)    |
        (1                              << DMA_COND_NEGATE_SHIFT)       |
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

    // Control on, and off immediately after, otherwise state machine will restart
    SPI_CTRL = control_on;
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    SPI_CTRL = control_rst;
    
    // Load four first dummy responses
    for (uint8_t i = 0; i < 4; i++) {

        // Stall while, RX buffer is empty
        while (SPI_FIFOSTAT & SPI_STATUS_RX_EMPTY_MASK);

        dummy = SPI_RX;
    }

    // Turning dma on to stream in rest of the data.
    // *dma_ctrl_reg = dma_controls;

    // DMA implements the following function:
    for (uint8_t i = 0; i < num_bytes; i++) {
        // Stall while, TX buffer full
        while (!(SPI_FIFOSTAT & SPI_STATUS_TX_ALMOST_FULL_MASK));

        // Write source array into SPI TX
        SPI_TX =  source_array[i];
    }

    // if (dma_busy()) {
    //     asm volatile ("wfi");
    // }

    // At the end clear, the read buffer.
    // Either put this in the irq handler, or check and clear at the beginning of all spi functions.
    // while (!(SPI_FIFOSTAT & SPI_STATUS_RX_EMPTY_MASK)) {
    //     dummy = SPI_RX;
    // }
}