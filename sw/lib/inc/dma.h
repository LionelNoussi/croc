#ifndef DMA_H
#define DMA_H

#include <stdint.h>

// -----------------------------------------------------------------------------
// DMA Address Map
// -----------------------------------------------------------------------------
#define DMA_BASE_ADDR               0x50000000

#define DMA_SRC_REG_OFFSET          0x0
#define DMA_TGT_REG_OFFSET          0x4
#define DMA_CONTROL_REG_OFFSET      0x8
#define DMA_CONDITION_REG_OFFSET    0xC
#define DMA_INTERRUPT_OFFSET        0x10
#define DMA_ACTIVATE_OFFSET         0x14
#define DMA_STATUS_OFFSET           0x18

#define DMA_REG(offset) ((volatile uint32_t*)(DMA_BASE_ADDR + (offset)))

// -----------------------------------------------------------------------------
// DMA Options (Control Register)
// -----------------------------------------------------------------------------

// Bit positions
#define DMA_CTRL_SRC_OFFSET_SHIFT         24
#define DMA_CTRL_DST_OFFSET_SHIFT         16
#define DMA_CTRL_NUM_TRANSFERS_SHIFT      5
#define DMA_CTRL_IRQ_ENABLE_SHIFT         4
#define DMA_CTRL_INC_SRC_SHIFT            3
#define DMA_CTRL_INC_DEST_SHIFT           2
#define DMA_CTRL_TRANSFER_SIZE_SHIFT      1
#define DMA_CTRL_ACTIVATE_SHIFT           0

// Bit masks
#define DMA_CTRL_SRC_OFFSET_MASK          0xFF
#define DMA_CTRL_DST_OFFSET_MASK          0xFF
#define DMA_CTRL_NUM_TRANSFERS_MASK       0x7FF
#define DMA_CTRL_IRQ_ENABLE_MASK          0x1
#define DMA_CTRL_INC_MASK                 0x1
#define DMA_CTRL_TRANSFER_SIZE_MASK       0x1
#define DMA_CTRL_ACTIVATE_MASK            0x1

// DMA Transfer Size Enum
typedef enum {
    DMA_TRANSFER_WORD = 0,
    DMA_TRANSFER_BYTE = 1
} dma_transfer_size_t;

// Struct to simplify programming
typedef struct {
    uint8_t src_offset;            // bits 31:24
    uint8_t dst_offset;            // bits 23:16
    uint16_t num_transfers;        // bits 15:5 (11 bits)
    uint8_t interrupt_enable;      // bit 4
    uint8_t increment_src;         // bit 3
    uint8_t increment_dst;         // bit 2
    dma_transfer_size_t transfer_size;      // bit 1
    uint8_t activate;
} dma_control_struct_t;

// Encodes DMA control word from control struct
typedef uint32_t dma_control_t;

// If 'activate' is 1, starts the transfer immediately.
dma_control_t encode_dma_controls(const dma_control_struct_t* opts);

// -----------------------------------------------------------------------------
// DMA Condition (Condition Register)
// -----------------------------------------------------------------------------

// Bit positions
#define DMA_COND_OFFSET_SHIFT             24
#define DMA_COND_MASK_SHIFT               16
#define DMA_COND_TYPE_SHIFT               2
#define DMA_COND_NEGATE_SHIFT             1
#define DMA_COND_ENABLE_SHIFT             0

// Bit masks
#define DMA_COND_OFFSET_MASK              0xFF
#define DMA_COND_MASK_MASK                0xFF
#define DMA_COND_TYPE_MASK                0x1
#define DMA_COND_NEGATE_MASK              0x1
#define DMA_COND_ENABLE_MASK              0x1

// Condition Type Enum
typedef enum {
    CONDITIONAL_READ = 0,
    CONDITIONAL_WRITE = 1
} conditional_type_t;

// Struct to simplify programming
typedef struct {
    uint8_t cond_addr_offset;              // bits 31:24
    uint8_t bitmask;                       // bits 23:16
    conditional_type_t conditional_type;   // bit 2
    uint8_t negate;                        // bit 1
    uint8_t enable;                        // bit 0
} dma_condition_struct_t;

// Encodes DMA condition word from condition struct
typedef uint32_t dma_condition_t;
dma_condition_t encode_dma_condition(const dma_condition_struct_t* cond);

// -----------------------------------------------------------------------------
// DMA STATUS
// -----------------------------------------------------------------------------

// Bit positions
#define DMA_STATUS_ACTIVE_MASK          0x1
#define DMA_STATUS_COMPLETED_RCVS_MASK  0X7FF
#define DMA_STATUS_COMPLETED_TRMS_MASK  0X7FF

// Bit masks
#define DMA_STATUS_ACTIVE_SHIFT         0
#define DMA_STATUS_COMPLETED_RCVS_SHIFT 21
#define DMA_STATUS_COMPLETED_TRMS_SHIFT 10

// Status Information Struct
typedef struct {
    uint16_t completed_receives;
    uint16_t completed_transmissions;
    uint8_t  active;
} dma_status_t;

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------

// Programs the DMA engine.
// Throws an error if the dma is currently busy
void program_dma(uint32_t src_addr, uint32_t dst_addr, dma_control_t options, dma_condition_t condition);

// Activate the dma (without having to specify any options)
// Throws an error if the DMA is currently busy
void activate_dma();

// Stop the DMA.
void interrupt_dma();

// Read the dma status
dma_status_t read_dma_status();

// Returns true if the dma is currently working
int dma_busy();

// -----------------------------------------------------------------------------
// Interrupts
// -----------------------------------------------------------------------------

// Machine-Interrupt-Enable Direct-Memory-Access Interrupt-Request Bit 
#define MIE_DMA_IRQ_BIT (1 << 19)

void enable_dma_irq(void);

void disable_dma_irq(void);

void dma_irq_handler() __attribute__((used, externally_visible));

#endif // DMA_H
