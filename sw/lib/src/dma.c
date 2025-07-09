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


static inline uint32_t encode_dma_options(dma_options_t opts, uint32_t activate) {
    return ((opts.src_offset        & DMA_CTRL_SRC_OFFSET_MASK)      << DMA_CTRL_SRC_OFFSET_SHIFT)     |
           ((opts.num_transfers     & DMA_CTRL_REPEAT_MASK)          << DMA_CTRL_REPEAT_SHIFT)         |
           ((opts.increment_source  & DMA_CTRL_INC_MASK)             << DMA_CTRL_INC_SRC_SHIFT)        |
           ((opts.increment_dest    & DMA_CTRL_INC_MASK)             << DMA_CTRL_INC_DEST_SHIFT)       |
           ((opts.size              & DMA_CTRL_TRANSFER_SIZE_MASK)   << DMA_CTRL_TRANSFER_SIZE_SHIFT)  |
           ((activate               & DMA_CTRL_ACTIVATE_MASK)        << DMA_CTRL_ACTIVATE_SHIFT);
}

static inline uint32_t encode_dma_condition(dma_condition_t cond) {
    return ((cond.cond_addr_offset  & DMA_COND_OFFSET_MASK)          << DMA_COND_OFFSET_SHIFT)         |
           ((cond.bitmask           & DMA_COND_MASK_MASK)            << DMA_COND_MASK_SHIFT)           |
           ((cond.negate            & DMA_COND_NEGATE_MASK)          << DMA_COND_NEGATE_SHIFT)         |
           ((cond.enable            & DMA_COND_ENABLE_MASK)          << DMA_COND_ENABLE_SHIFT);
}

void program_dma(const dma_config_t* cfg, uint32_t activate) {
    *dma_src_reg    = cfg->src_addr;
    *dma_dst_reg    = cfg->dest_addr;
    *dma_cond_reg   = encode_dma_condition(cfg->condition);
    *dma_ctrl_reg   = encode_dma_options(cfg->options, activate);
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
        .active = (raw_status >> DMA_STATUS_ACTIVE_SHIFT) & DMA_STATUS_ACTIVE_MASK
    };

    return status;
}

int dma_busy() {
    return *dma_status_reg & DMA_STATUS_ACTIVE_MASK;
}

int dma_ready() {
    return !(*dma_status_reg & DMA_STATUS_ACTIVE_MASK);
}