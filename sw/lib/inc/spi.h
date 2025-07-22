#pragma once


#include <stdint.h>
#include "config.h"

#define SPI_CTRL            (*(volatile uint32_t *)(SPI_BASE_ADDR + 0x00))
#define SPI_STATUS          (*(volatile uint32_t *)(SPI_BASE_ADDR + 0x04))

// TXRX buffer access (byte by byte, 32 entries)
#define SPI_TXRX(i)         (*(volatile uint8_t  *)(SPI_BASE_ADDR + 0x08 + (i)))  // i ∈ [0, 31]

// Address (16-bit value, stored as 2 bytes at 0x28 and 0x29)
#define SPI_ADDR_LO         (*(volatile uint8_t  *)(SPI_BASE_ADDR + 0x28))
#define SPI_ADDR_HI         (*(volatile uint8_t  *)(SPI_BASE_ADDR + 0x29))

// Length (1 byte at 0x2A)
#define SPI_LENGTH          (*(volatile uint8_t  *)(SPI_BASE_ADDR + 0x2A))

uint8_t spi_write(uint8_t data);