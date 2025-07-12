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
           ((cond->conditional_type  & DMA_COND_TYPE_MASK)            << DMA_COND_TYPE_SHIFT)           |
           ((cond->negate            & DMA_COND_NEGATE_MASK)          << DMA_COND_NEGATE_SHIFT)         |
           ((cond->enable            & DMA_COND_ENABLE_MASK)          << DMA_COND_ENABLE_SHIFT);
}


void program_dma(uint32_t src_addr, uint32_t dst_addr, dma_control_t options, dma_condition_t condition) {
    *dma_src_reg    = src_addr;
    *dma_dst_reg    = dst_addr;
    *dma_cond_reg   = condition;
    *dma_ctrl_reg   = options;
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

void dma_irq_handler() {
    *DMA_REG(DMA_INTERRUPT_OFFSET);
}