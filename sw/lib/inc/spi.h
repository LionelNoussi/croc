#pragma once

#include <stdint.h>
#include "config.h"

#define SPI_CTRL        (*(volatile uint8_t *)(SPI_BASE_ADDR + 0x000))
#define SPI_STATUS      (*(volatile uint8_t *)(SPI_BASE_ADDR + 0x004))
#define SPI_TX(i)       (*(volatile uint8_t *)(SPI_BASE_ADDR + 0x008 + (i)))
#define SPI_RX(i)       (*(volatile uint8_t *)(SPI_BASE_ADDR + 0x018 + (i)))
#define SPI_ADDR_LO     (*(volatile uint8_t *)(SPI_BASE_ADDR + 0x028))
#define SPI_ADDR_HI     (*(volatile uint8_t *)(SPI_BASE_ADDR + 0x029))
#define SPI_LENGTH      (*(volatile uint8_t *)(SPI_BASE_ADDR + 0x02C))


// SPI read/write interface
void spi_write(uint16_t addr, const uint8_t *data, uint8_t length);
void spi_read(uint16_t addr, uint8_t *data, uint8_t length);
