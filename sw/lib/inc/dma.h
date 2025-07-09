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
#define DMA_CTRL_REPEAT_SHIFT             13
#define DMA_CTRL_INC_SRC_SHIFT            3
#define DMA_CTRL_INC_DEST_SHIFT           2
#define DMA_CTRL_TRANSFER_SIZE_SHIFT      1
#define DMA_CTRL_ACTIVATE_SHIFT           0

// Bit masks
#define DMA_CTRL_SRC_OFFSET_MASK          0xFF
#define DMA_CTRL_REPEAT_MASK              0x7FF
#define DMA_CTRL_INC_MASK                 0x1
#define DMA_CTRL_TRANSFER_SIZE_MASK       0x1
#define DMA_CTRL_ACTIVATE_MASK            0x1

// DMA Transfer Size Enum
typedef enum {
    DMA_TRANSFER_WORD = 0,
    DMA_TRANSFER_BYTE = 1
} dma_transfer_size_t;

typedef struct {
    uint32_t src_offset;           // bits 31:24
    uint32_t num_transfers;        // bits 23:13 (11 bits)
    uint32_t increment_source;     // bit 3
    uint32_t increment_dest;       // bit 2
    dma_transfer_size_t size;      // bit 1
} dma_options_t;

// -----------------------------------------------------------------------------
// DMA Condition (Condition Register)
// -----------------------------------------------------------------------------

// Bit positions
#define DMA_COND_OFFSET_SHIFT             24
#define DMA_COND_MASK_SHIFT               16
#define DMA_COND_NEGATE_SHIFT             1
#define DMA_COND_ENABLE_SHIFT             0

// Bit masks
#define DMA_COND_OFFSET_MASK              0xFF
#define DMA_COND_MASK_MASK                0xFF
#define DMA_COND_NEGATE_MASK              0x1
#define DMA_COND_ENABLE_MASK              0x1

typedef struct {
    uint32_t cond_addr_offset;   // bits 31:24
    uint32_t bitmask;      // bits 23:16
    uint32_t negate;       // bit 1
    uint32_t enable;       // bit 0
} dma_condition_t;

// -----------------------------------------------------------------------------
// Top-Level DMA Configuration Struct
// -----------------------------------------------------------------------------

typedef struct {
    uint32_t src_addr;
    uint32_t dest_addr;
    dma_options_t options;
    dma_condition_t condition;
} dma_config_t;

// -----------------------------------------------------------------------------
// DMA STATUS
// -----------------------------------------------------------------------------

// Bit positions
#define DMA_STATUS_ACTIVE_MASK   0x1

// Bit masks
#define DMA_STATUS_ACTIVE_SHIFT  0

// TODO Create support for more status information
typedef struct {
    uint32_t active : 1;  // LSB: 1 if DMA is running, 0 if idle
} dma_status_t;

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------

// Encodes DMA control word from options struct
static inline uint32_t encode_dma_options(dma_options_t opts, uint32_t activate);

// Encodes DMA condition word from condition struct
static inline uint32_t encode_dma_condition(dma_condition_t cond);

// Program the DMA engine. If 'activate' is 1, starts the transfer immediately.
// Throws an error if the dma is currently busy
void program_dma(const dma_config_t* cfg, uint32_t activate);

// Activate the dma (without having to specify any options)
// Throws an error if the DMA is currently busy
void activate_dma();

// Stop the DMA.
void interrupt_dma();

// Read the dma status
dma_status_t read_dma_status();

// Returns true if the dma is currently working
int dma_busy();

// Returns true if the dma is currently not working
int dma_ready();

#endif // DMA_H
